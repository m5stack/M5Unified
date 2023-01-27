#include <SD.h>
#include <M5Unified.h>

#include <esp_log.h>

static constexpr const gpio_num_t SDCARD_CSPIN = GPIO_NUM_4;

static constexpr const char* files[] = {
  "/file1.wav",
  "/file2.wav",
  "/file3.wav",
};

static constexpr const size_t buf_num = 3;
static constexpr const size_t buf_size = 1024;
static uint8_t wav_data[buf_num][buf_size];

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

static bool playSdWav(const char* filename)
{
  auto file = SD.open(filename);

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
    size_t len = data_len < buf_size ? data_len : buf_size;
    len = file.read(wav_data[idx], len);
    data_len -= len;

    if (flg_16bit) {
      M5.Speaker.playRaw((const int16_t*)wav_data[idx], len >> 1, wav_header.sample_rate, wav_header.channel > 1, 1, 0);
    } else {
      M5.Speaker.playRaw((const uint8_t*)wav_data[idx], len, wav_header.sample_rate, wav_header.channel > 1, 1, 0);
    }
    idx = idx < (buf_num - 1) ? idx + 1 : 0;
  }
  file.close();

  return true;
}

void setup(void)
{
  M5.begin();

  SD.begin(SDCARD_CSPIN, SPI, 25000000);

  // M5.Speaker.setVolume(32);
}

void loop(void)
{
  for (auto filename : files) {
    playSdWav(filename);
    delay(500);
  }
}
