#include <SD.h>
#include <soc/soc_caps.h>
#if __has_include(<SD_MMC.h>) && defined (SOC_SDMMC_HOST_SUPPORTED)
#include <SD_MMC.h>
#define WAV_HAS_SD_MMC
#endif
#include <M5Unified.h>
#include <atomic>

#include <esp_log.h>

static fs::FS* card = nullptr; // &SD_MMC or &SD, whichever mountCard() opened

/// Two buffers used alternately. A buffer is refilled only after the speaker
/// task has released it, which it reports through setBufferReleaseCallback.
static constexpr const size_t buf_num = 2;
static constexpr const size_t buf_size = 1024;
static uint8_t wav_data[buf_num][buf_size];
// These flags are shared between tasks. Use std::atomic<bool> here;
// plain bool or volatile does not safely share updates between tasks.
static std::atomic<bool> wav_busy[buf_num] = { {false}, {false} };

static void wav_released(void*, const void* data, uint8_t)
{ // The speaker has finished using this buffer; it can be refilled now.
  for (size_t i = 0; i < buf_num; ++i) { if (data == wav_data[i]) { wav_busy[i] = false; } }
}

struct __attribute__((packed)) wav_header_t
{
  char RIFF[4];
  uint32_t chunk_size;
  char WAVEfmt[8];
  uint32_t fmt_chunk_size;
  uint16_t audiofmt;
  uint16_t channel;
  uint32_t sample_rate;
  uint32_t byte_per_sec;
  uint16_t block_size;
  uint16_t bit_per_sample;
};

struct __attribute__((packed)) sub_chunk_t
{
  char identifier[4];
  uint32_t chunk_size;
  uint8_t data[1];
};

/// Mount the card with the pins M5Unified knows for the board: 4-bit SD_MMC
/// where the extra data lines are wired (Tab5), SPI elsewhere. CS falls back
/// to G4 when the table has none (ATOMIC Speaker base, boards without an entry).
static fs::FS* mountCard(void)
{
#if defined (WAV_HAS_SD_MMC)
  if (M5.hasSDMMC())
  {
    // on a fixed IO_MUX slot (ESP32-P4 slot 0) setPins rejects other pins; the core logs why
    if (!SD_MMC.setPins(M5.getPin(m5::pin_name_t::sd_mmc_clk), M5.getPin(m5::pin_name_t::sd_mmc_cmd), M5.getPin(m5::pin_name_t::sd_mmc_d0)
                      , M5.getPin(m5::pin_name_t::sd_mmc_d1),  M5.getPin(m5::pin_name_t::sd_mmc_d2),  M5.getPin(m5::pin_name_t::sd_mmc_d3)))
    {
      return nullptr;
    }
    return SD_MMC.begin("/sdcard", false) ? (fs::FS*)&SD_MMC : nullptr;
  }
#endif
  int cs = M5.getPin(m5::pin_name_t::sd_spi_cs);
  if (cs < 0) { cs = GPIO_NUM_4; }
  if (M5.hasSD())
  {
    SPI.begin(M5.getPin(m5::pin_name_t::sd_spi_sclk), M5.getPin(m5::pin_name_t::sd_spi_miso), M5.getPin(m5::pin_name_t::sd_spi_mosi), cs);
  }
  return SD.begin(cs, SPI, 25000000) ? (fs::FS*)&SD : nullptr;
}

static bool playSdWav(const char* filename)
{
  auto file = card->open(filename);

  if (!file) { return false; }

  wav_header_t wav_header;
  file.read((uint8_t*)&wav_header, sizeof(wav_header_t));

  ESP_LOGD("wav", "RIFF           : %.4s" , wav_header.RIFF          );
  ESP_LOGD("wav", "chunk_size     : %d"   , wav_header.chunk_size    );
  ESP_LOGD("wav", "WAVEfmt        : %.8s" , wav_header.WAVEfmt       );
  ESP_LOGD("wav", "fmt_chunk_size : %d"   , wav_header.fmt_chunk_size);
  ESP_LOGD("wav", "audiofmt       : %d"   , wav_header.audiofmt      );
  ESP_LOGD("wav", "channel        : %d"   , wav_header.channel       );
  ESP_LOGD("wav", "sample_rate    : %d"   , wav_header.sample_rate   );
  ESP_LOGD("wav", "byte_per_sec   : %d"   , wav_header.byte_per_sec  );
  ESP_LOGD("wav", "block_size     : %d"   , wav_header.block_size    );
  ESP_LOGD("wav", "bit_per_sample : %d"   , wav_header.bit_per_sample);

  if ( memcmp(wav_header.RIFF,    "RIFF",     4)
    || memcmp(wav_header.WAVEfmt, "WAVEfmt ", 8)
    || wav_header.audiofmt != 1
    || wav_header.bit_per_sample < 8
    || wav_header.bit_per_sample > 16
    || wav_header.channel == 0
    || wav_header.channel > 2
    )
  {
    file.close();
    return false;
  }

  file.seek(offsetof(wav_header_t, audiofmt) + wav_header.fmt_chunk_size);
  sub_chunk_t sub_chunk;

  file.read((uint8_t*)&sub_chunk, 8);

  ESP_LOGD("wav", "sub id         : %.4s" , sub_chunk.identifier);
  ESP_LOGD("wav", "sub chunk_size : %d"   , sub_chunk.chunk_size);

  while(memcmp(sub_chunk.identifier, "data", 4))
  {
    if (!file.seek(sub_chunk.chunk_size, SeekMode::SeekCur)) { break; }
    file.read((uint8_t*)&sub_chunk, 8);

    ESP_LOGD("wav", "sub id         : %.4s" , sub_chunk.identifier);
    ESP_LOGD("wav", "sub chunk_size : %d"   , sub_chunk.chunk_size);
  }

  if (memcmp(sub_chunk.identifier, "data", 4))
  {
    file.close();
    return false;
  }

  int32_t data_len = sub_chunk.chunk_size;
  bool flg_16bit = (wav_header.bit_per_sample >> 4);

  size_t idx = 0;
  while (data_len > 0) {
    while (wav_busy[idx]) { M5.delay(1); } // wait until the speaker task has released this buffer
    size_t len = data_len < buf_size ? data_len : buf_size;
    len = file.read(wav_data[idx], len);
    if (flg_16bit) { len &= ~(size_t)1; }
    if (len == 0) { break; } // file shorter than the data chunk claims
    data_len -= len;

    // busy is set before playRaw: the release can only come after the
    // request is queued. playRaw returns false when nothing was queued.
    wav_busy[idx] = true;
    bool ok;
    if (flg_16bit) {
      ok = M5.Speaker.playRaw((const int16_t*)wav_data[idx], len >> 1, wav_header.sample_rate, wav_header.channel > 1, 1, 0);
    } else {
      ok = M5.Speaker.playRaw((const uint8_t*)wav_data[idx], len, wav_header.sample_rate, wav_header.channel > 1, 1, 0);
    }
    if (!ok) { wav_busy[idx] = false; }
    idx = idx < (buf_num - 1) ? idx + 1 : 0;
  }
  file.close();

  return true;
}

void setup(void)
{
  M5.begin();
  M5.Log.setLogLevel(m5::log_target_serial, ESP_LOG_INFO); // so the file names below reach the log

  M5.Speaker.setBufferReleaseCallback(nullptr, wav_released);
  card = mountCard();

  // M5.Speaker.setVolume(32);
}

void loop(void)
{
  // Play every *.wav in the root of the card, in directory order.
  if (!card) { card = mountCard(); } // a card inserted after start
  auto dir = card ? card->open("/") : fs::File();
  if (!dir) {
    M5.Display.println("no SD card");
    M5.delay(1000);
    return;
  }
  size_t played = 0;
  for (auto file = dir.openNextFile(); file; file = dir.openNextFile()) {
    bool is_dir = file.isDirectory();
    String name = file.name(); // a bare name or a full path, depending on the core version
    file.close();
    String lower = name;
    lower.toLowerCase();
    if (is_dir || !lower.endsWith(".wav")) { continue; }
    String path = name.startsWith("/") ? name : "/" + name;
    M5.Display.println(path);
    M5_LOGI("%s", path.c_str());
    if (playSdWav(path.c_str())) { ++played; }
    M5.delay(500);
  }
  dir.close();
  if (played == 0) {
    M5.Display.println("no *.wav found");
    M5.delay(1000);
  }
}
