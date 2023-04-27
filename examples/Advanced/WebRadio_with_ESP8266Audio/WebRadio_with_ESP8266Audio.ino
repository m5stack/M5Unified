

#define WIFI_SSID "SET YOUR WIFI SSID"
#define WIFI_PASS "SET YOUR WIFI PASS"


#include <HTTPClient.h>
#include <math.h>

/// need ESP8266Audio library. ( URL : https://github.com/earlephilhower/ESP8266Audio/ )
#include <AudioOutput.h>
#include <AudioFileSourceICYStream.h>
#include <AudioFileSource.h>
#include <AudioFileSourceBuffer.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>

#include <M5UnitLCD.h>
#include <M5UnitOLED.h>
#include <M5Unified.h>

/// set M5Speaker virtual channel (0-7)
static constexpr uint8_t m5spk_virtual_channel = 0;

/// set web radio station url
static constexpr const char* station_list[][2] =
{
  {"MAXXED Out"        , "http://149.56.195.94:8015/steam"},
  {"Asia Dream"        , "http://igor.torontocast.com:1025/;.-mp3"},
  {"thejazzstream"     , "http://wbgo.streamguys.net/thejazzstream"},
  {"181-beatles_128k"  , "http://listen.181fm.com/181-beatles_128k.mp3"},
  {"illstreet-128-mp3" , "http://ice1.somafm.com/illstreet-128-mp3"},
  {"bootliquor-128-mp3", "http://ice1.somafm.com/bootliquor-128-mp3"},
  {"dronezone-128-mp3" , "http://ice1.somafm.com/dronezone-128-mp3"},
  {"Lite Favorites"    , "http://naxos.cdnstream.com:80/1255_128"},
  {"Classic FM"        , "http://media-ice.musicradio.com:80/ClassicFMMP3"},
};
static constexpr const size_t stations = sizeof(station_list) / sizeof(station_list[0]);

class AudioOutputM5Speaker : public AudioOutput
{
  public:
    AudioOutputM5Speaker(m5::Speaker_Class* m5sound, uint8_t virtual_sound_channel = 0)
    {
      _m5sound = m5sound;
      _virtual_ch = virtual_sound_channel;
    }
    virtual ~AudioOutputM5Speaker(void) {};
    virtual bool begin(void) override { return true; }
    virtual bool ConsumeSample(int16_t sample[2]) override
    {
      if (_tri_buffer_index < tri_buf_size)
      {
        _tri_buffer[_tri_index][_tri_buffer_index  ] = sample[0];
        _tri_buffer[_tri_index][_tri_buffer_index+1] = sample[1];
        _tri_buffer_index += 2;

        return true;
      }

      flush();
      return false;
    }
    virtual void flush(void) override
    {
      if (_tri_buffer_index)
      {
        _m5sound->playRaw(_tri_buffer[_tri_index], _tri_buffer_index, hertz, true, 1, _virtual_ch);
        _tri_index = _tri_index < 2 ? _tri_index + 1 : 0;
        _tri_buffer_index = 0;
        ++_update_count;
      }
    }
    virtual bool stop(void) override
    {
      flush();
      _m5sound->stop(_virtual_ch);
      for (size_t i = 0; i < 3; ++i)
      {
        memset(_tri_buffer[i], 0, tri_buf_size * sizeof(int16_t));
      }
      ++_update_count;
      return true;
    }

    const int16_t* getBuffer(void) const { return _tri_buffer[(_tri_index + 2) % 3]; }
    const uint32_t getUpdateCount(void) const { return _update_count; }

  protected:
    m5::Speaker_Class* _m5sound;
    uint8_t _virtual_ch;
    static constexpr size_t tri_buf_size = 640;
    int16_t _tri_buffer[3][tri_buf_size];
    size_t _tri_buffer_index = 0;
    size_t _tri_index = 0;
    size_t _update_count = 0;
};


#define FFT_SIZE 256
class fft_t
{
  float _wr[FFT_SIZE + 1];
  float _wi[FFT_SIZE + 1];
  float _fr[FFT_SIZE + 1];
  float _fi[FFT_SIZE + 1];
  uint16_t _br[FFT_SIZE + 1];
  size_t _ie;

public:
  fft_t(void)
  {
#ifndef M_PI
#define M_PI 3.141592653
#endif
    _ie = logf( (float)FFT_SIZE ) / log(2.0) + 0.5;
    static constexpr float omega = 2.0f * M_PI / FFT_SIZE;
    static constexpr int s4 = FFT_SIZE / 4;
    static constexpr int s2 = FFT_SIZE / 2;
    for ( int i = 1 ; i < s4 ; ++i)
    {
    float f = cosf(omega * i);
      _wi[s4 + i] = f;
      _wi[s4 - i] = f;
      _wr[     i] = f;
      _wr[s2 - i] = -f;
    }
    _wi[s4] = _wr[0] = 1;

    size_t je = 1;
    _br[0] = 0;
    _br[1] = FFT_SIZE / 2;
    for ( size_t i = 0 ; i < _ie - 1 ; ++i )
    {
      _br[ je << 1 ] = _br[ je ] >> 1;
      je = je << 1;
      for ( size_t j = 1 ; j < je ; ++j )
      {
        _br[je + j] = _br[je] + _br[j];
      }
    }
  }

  void exec(const int16_t* in)
  {
    memset(_fi, 0, sizeof(_fi));
    for ( size_t j = 0 ; j < FFT_SIZE / 2 ; ++j )
    {
      float basej = 0.25 * (1.0-_wr[j]);
      size_t r = FFT_SIZE - j - 1;

      /// perform han window and stereo to mono convert.
      _fr[_br[j]] = basej * (in[j * 2] + in[j * 2 + 1]);
      _fr[_br[r]] = basej * (in[r * 2] + in[r * 2 + 1]);
    }

    size_t s = 1;
    size_t i = 0;
    do
    {
      size_t ke = s;
      s <<= 1;
      size_t je = FFT_SIZE / s;
      size_t j = 0;
      do
      {
        size_t k = 0;
        do
        {
          size_t l = s * j + k;
          size_t m = ke * (2 * j + 1) + k;
          size_t p = je * k;
          float Wxmr = _fr[m] * _wr[p] + _fi[m] * _wi[p];
          float Wxmi = _fi[m] * _wr[p] - _fr[m] * _wi[p];
          _fr[m] = _fr[l] - Wxmr;
          _fi[m] = _fi[l] - Wxmi;
          _fr[l] += Wxmr;
          _fi[l] += Wxmi;
        } while ( ++k < ke) ;
      } while ( ++j < je );
    } while ( ++i < _ie );
  }

  uint32_t get(size_t index)
  {
    return (index < FFT_SIZE / 2) ? (uint32_t)sqrtf(_fr[ index ] * _fr[ index ] + _fi[ index ] * _fi[ index ]) : 0u;
  }
};

static constexpr const int preallocateBufferSize = 5 * 1024;
static constexpr const int preallocateCodecSize = 29192; // MP3 codec max mem needed
static void* preallocateBuffer = nullptr;
static void* preallocateCodec = nullptr;
static constexpr size_t WAVE_SIZE = 320;
static AudioOutputM5Speaker out(&M5.Speaker, m5spk_virtual_channel);
static AudioGenerator *decoder = nullptr;
static AudioFileSourceICYStream *file = nullptr;
static AudioFileSourceBuffer *buff = nullptr;
static fft_t fft;
static bool fft_enabled = false;
static bool wave_enabled = false;
static uint16_t prev_y[(FFT_SIZE / 2)+1];
static uint16_t peak_y[(FFT_SIZE / 2)+1];
static int16_t wave_y[WAVE_SIZE];
static int16_t wave_h[WAVE_SIZE];
static int16_t raw_data[WAVE_SIZE * 2];
static int header_height = 0;
static size_t station_index = 0;
static char stream_title[128] = { 0 };
static const char* meta_text[2] = { nullptr, stream_title };
static const size_t meta_text_num = sizeof(meta_text) / sizeof(meta_text[0]);
static uint8_t meta_mod_bits = 0;
static volatile size_t playindex = ~0u;

static void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string)
{
  (void)cbData;
  if ((strcmp(type, "StreamTitle") == 0) && (strcmp(stream_title, string) != 0))
  {
    strncpy(stream_title, string, sizeof(stream_title));
    meta_mod_bits |= 2;
  }
}

static void stop(void)
{
  if (decoder) {
    decoder->stop();
    delete decoder;
    decoder = nullptr;
  }

  if (buff) {
    buff->close();
    delete buff;
    buff = nullptr;
  }
  if (file) {
    file->close();
    delete file;
    file = nullptr;
  }
  out.stop();
}

static void play(size_t index)
{
  playindex = index;
}

static void decodeTask(void*)
{
  for (;;)
  {
    delay(1);
    if (playindex != ~0u)
    {
      auto index = playindex;
      playindex = ~0u;
      stop();
      meta_text[0] = station_list[index][0];
      stream_title[0] = 0;
      meta_mod_bits = 3;
      file = new AudioFileSourceICYStream(station_list[index][1]);
      file->RegisterMetadataCB(MDCallback, (void*)"ICY");
      buff = new AudioFileSourceBuffer(file, preallocateBuffer, preallocateBufferSize);
      decoder = new AudioGeneratorMP3(preallocateCodec, preallocateCodecSize);
      decoder->begin(buff, &out);
    }
    if (decoder && decoder->isRunning())
    {
      if (!decoder->loop()) { decoder->stop(); }
    }
  }
}

static uint32_t bgcolor(LGFX_Device* gfx, int y)
{
  auto h = gfx->height();
  auto dh = h - header_height;
  int v = ((h - y)<<5) / dh;
  if (dh > 44)
  {
    int v2 = ((h - y - 1)<<5) / dh;
    if ((v >> 2) != (v2 >> 2))
    {
      return 0x666666u;
    }
  }
  return gfx->color888(v + 2, v, v + 6);
}

static void gfxSetup(LGFX_Device* gfx)
{
  if (gfx == nullptr) { return; }
  if (gfx->width() < gfx->height())
  {
    gfx->setRotation(gfx->getRotation()^1);
  }
  gfx->setFont(&fonts::lgfxJapanGothic_12);
  gfx->setEpdMode(epd_mode_t::epd_fastest);
  gfx->setTextWrap(false);
  gfx->setCursor(0, 8);
  gfx->println("WebRadio player");
  gfx->fillRect(0, 6, gfx->width(), 2, TFT_BLACK);

  header_height = (gfx->height() > 80) ? 33 : 21;
  fft_enabled = !gfx->isEPD();
  if (fft_enabled)
  {
    wave_enabled = (gfx->getBoard() != m5gfx::board_M5UnitLCD);

    for (int y = header_height; y < gfx->height(); ++y)
    {
      gfx->drawFastHLine(0, y, gfx->width(), bgcolor(gfx, y));
    }
  }

  for (int x = 0; x < (FFT_SIZE/2)+1; ++x)
  {
    prev_y[x] = INT16_MAX;
    peak_y[x] = INT16_MAX;
  }
  for (int x = 0; x < WAVE_SIZE; ++x)
  {
    wave_y[x] = gfx->height();
    wave_h[x] = 0;
  }
}

void gfxLoop(LGFX_Device* gfx)
{
  if (gfx == nullptr) { return; }
  if (header_height > 32)
  {
    if (meta_mod_bits)
    {
      gfx->startWrite();
      for (int id = 0; id < meta_text_num; ++id)
      {
        if (0 == (meta_mod_bits & (1<<id))) { continue; }
        meta_mod_bits &= ~(1<<id);
        size_t y = id * 12;
        if (y+12 >= header_height) { continue; }
        gfx->setCursor(4, 8 + y);
        gfx->fillRect(0, 8 + y, gfx->width(), 12, gfx->getBaseColor());
        gfx->print(meta_text[id]);
        gfx->print(" "); // Garbage data removal when UTF8 characters are broken in the middle.
      }
      gfx->display();
      gfx->endWrite();
    }
  }
  else
  {
    static int title_x;
    static int title_id;
    static int wait = INT16_MAX;

    if (meta_mod_bits)
    {
      if (meta_mod_bits & 1)
      {
        title_x = 4;
        title_id = 0;
        gfx->fillRect(0, 8, gfx->width(), 12, gfx->getBaseColor());
      }
      meta_mod_bits = 0;
      wait = 0;
    }

    if (--wait < 0)
    {
      int tx = title_x;
      int tid = title_id;
      wait = 3;
      gfx->startWrite();
      uint_fast8_t no_data_bits = 0;
      do
      {
        if (tx == 4) { wait = 255; }
        gfx->setCursor(tx, 8);
        const char* meta = meta_text[tid];
        if (meta[0] != 0)
        {
          gfx->print(meta);
          gfx->print("  /  ");
          tx = gfx->getCursorX();
          if (++tid == meta_text_num) { tid = 0; }
          if (tx <= 4)
          {
            title_x = tx;
            title_id = tid;
          }
        }
        else
        {
          if ((no_data_bits |= 1 << tid) == ((1 << meta_text_num) - 1))
          {
            break;
          }
          if (++tid == meta_text_num) { tid = 0; }
        }
      } while (tx < gfx->width());
      --title_x;
      gfx->display();
      gfx->endWrite();
    }
  }

  if (fft_enabled)
  {
    static int prev_x[2];
    static int peak_x[2];

    auto buf = out.getBuffer();
    if (buf)
    {
      memcpy(raw_data, buf, WAVE_SIZE * 2 * sizeof(int16_t)); // stereo data copy
      gfx->startWrite();

      // draw stereo level meter
      for (size_t i = 0; i < 2; ++i)
      {
        int32_t level = 0;
        for (size_t j = i; j < 640; j += 32)
        {
          uint32_t lv = abs(raw_data[j]);
          if (level < lv) { level = lv; }
        }

        int32_t x = (level * gfx->width()) / INT16_MAX;
        int32_t px = prev_x[i];
        if (px != x)
        {
          gfx->fillRect(x, i * 3, px - x, 2, px < x ? 0xFF9900u : 0x330000u);
          prev_x[i] = x;
        }
        px = peak_x[i];
        if (px > x)
        {
          gfx->writeFastVLine(px, i * 3, 2, TFT_BLACK);
          px--;
        }
        else
        {
          px = x;
        }
        if (peak_x[i] != px)
        {
          peak_x[i] = px;
          gfx->writeFastVLine(px, i * 3, 2, TFT_WHITE);
        }
      }
      gfx->display();

      // draw FFT level meter
      fft.exec(raw_data);
      size_t bw = gfx->width() / 60;
      if (bw < 3) { bw = 3; }
      int32_t dsp_height = gfx->height();
      int32_t fft_height = dsp_height - header_height - 1;
      size_t xe = gfx->width() / bw;
      if (xe > (FFT_SIZE/2)) { xe = (FFT_SIZE/2); }
      int32_t wave_next = ((header_height + dsp_height) >> 1) + (((256 - (raw_data[0] + raw_data[1])) * fft_height) >> 17);

      uint32_t bar_color[2] = { 0x000033u, 0x99AAFFu };

      for (size_t bx = 0; bx <= xe; ++bx)
      {
        size_t x = bx * bw;
        if ((x & 7) == 0) { gfx->display(); taskYIELD(); }
        int32_t f = fft.get(bx);
        int32_t y = (f * fft_height) >> 18;
        if (y > fft_height) { y = fft_height; }
        y = dsp_height - y;
        int32_t py = prev_y[bx];
        if (y != py)
        {
          gfx->fillRect(x, y, bw - 1, py - y, bar_color[(y < py)]);
          prev_y[bx] = y;
        }
        py = peak_y[bx] + 1;
        if (py < y)
        {
          gfx->writeFastHLine(x, py - 1, bw - 1, bgcolor(gfx, py - 1));
        }
        else
        {
          py = y - 1;
        }
        if (peak_y[bx] != py)
        {
          peak_y[bx] = py;
          gfx->writeFastHLine(x, py, bw - 1, TFT_WHITE);
        }


        if (wave_enabled)
        {
          for (size_t bi = 0; bi < bw; ++bi)
          {
            size_t i = x + bi;
            if (i >= gfx->width() || i >= WAVE_SIZE) { break; }
            y = wave_y[i];
            int32_t h = wave_h[i];
            bool use_bg = (bi+1 == bw);
            if (h>0)
            { /// erase previous wave.
              gfx->setAddrWindow(i, y, 1, h);
              h += y;
              do
              {
                uint32_t bg = (use_bg || y < peak_y[bx]) ? bgcolor(gfx, y)
                            : (y == peak_y[bx]) ? 0xFFFFFFu
                            : bar_color[(y >= prev_y[bx])];
                gfx->writeColor(bg, 1);
              } while (++y < h);
            }
            size_t i2 = i << 1;
            int32_t y1 = wave_next;
            wave_next = ((header_height + dsp_height) >> 1) + (((256 - (raw_data[i2] + raw_data[i2 + 1])) * fft_height) >> 17);
            int32_t y2 = wave_next;
            if (y1 > y2)
            {
              int32_t tmp = y1;
              y1 = y2;
              y2 = tmp;
            }
            y = y1;
            h = y2 + 1 - y;
            wave_y[i] = y;
            wave_h[i] = h;
            if (h>0)
            { /// draw new wave.
              gfx->setAddrWindow(i, y, 1, h);
              h += y;
              do
              {
                uint32_t bg = (y < prev_y[bx]) ? 0xFFCC33u : 0xFFFFFFu;
                gfx->writeColor(bg, 1);
              } while (++y < h);
            }
          }
        }
      }
      gfx->display();
      gfx->endWrite();
    }
  }

  if (!gfx->displayBusy())
  { // draw volume bar
    static int px;
    uint8_t v = M5.Speaker.getVolume();
    int x = v * (gfx->width()) >> 8;
    if (px != x)
    {
      gfx->fillRect(x, 6, px - x, 2, px < x ? 0xAAFFAAu : 0u);
      gfx->display();
      px = x;
    }
  }
}

void setup(void)
{
  auto cfg = M5.config();

  // If you want to play sound from ModuleDisplay, write this
//  cfg.external_speaker.module_display = true;

  // If you want to play sound from ModuleRCA, write this
//  cfg.external_speaker.module_rca     = true;

  // If you want to play sound from HAT Speaker, write this
  cfg.external_speaker.hat_spk        = true;

  // If you want to play sound from HAT Speaker2, write this
//  cfg.external_speaker.hat_spk2       = true;

  // If you want to play sound from ATOMIC Speaker, write this
  cfg.external_speaker.atomic_spk     = true;

  M5.begin(cfg);

  preallocateBuffer = malloc(preallocateBufferSize);
  preallocateCodec = malloc(preallocateCodecSize);
  if (!preallocateBuffer || !preallocateCodec) {
    M5.Display.printf("FATAL ERROR:  Unable to preallocate %d bytes for app\n", preallocateBufferSize + preallocateCodecSize);
    for (;;) { delay(1000); }
  }

  { /// custom setting
    auto spk_cfg = M5.Speaker.config();
    /// Increasing the sample_rate will improve the sound quality instead of increasing the CPU load.
    spk_cfg.sample_rate = 96000; // default:64000 (64kHz)  e.g. 48000 , 50000 , 80000 , 96000 , 100000 , 128000 , 144000 , 192000 , 200000
    spk_cfg.task_pinned_core = APP_CPU_NUM;
    M5.Speaker.config(spk_cfg);
  }


  M5.Speaker.begin();

  M5.Display.println("Connecting to WiFi");
  WiFi.disconnect();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);

#if defined ( WIFI_SSID ) &&  defined ( WIFI_PASS )
  WiFi.begin(WIFI_SSID, WIFI_PASS);
#else
  WiFi.begin();
#endif

  // Try forever
  while (WiFi.status() != WL_CONNECTED) {
    M5.Display.print(".");
    delay(100);
  }
  M5.Display.clear();

  gfxSetup(&M5.Display);

  play(station_index);

  xTaskCreatePinnedToCore(decodeTask, "decodeTask", 4096, nullptr, 1, nullptr, PRO_CPU_NUM);
}

void loop(void)
{
  gfxLoop(&M5.Display);

  {
    static int prev_frame;
    int frame;
    do
    {
      delay(1);
    } while (prev_frame == (frame = millis() >> 3)); /// 8 msec cycle wait
    prev_frame = frame;
  }

  M5.update();
  if (M5.BtnA.wasPressed())
  {
    M5.Speaker.tone(440, 50);
  }
  if (M5.BtnA.wasDecideClickCount())
  {
    switch (M5.BtnA.getClickCount())
    {
    case 1:
      M5.Speaker.tone(1000, 100);
      if (++station_index >= stations) { station_index = 0; }
      play(station_index);
      break;

    case 2:
      M5.Speaker.tone(800, 100);
      if (station_index == 0) { station_index = stations; }
      play(--station_index);
      break;
    }
  }
  if (M5.BtnA.isHolding() || M5.BtnB.isPressed() || M5.BtnC.isPressed())
  {
    size_t v = M5.Speaker.getVolume();
    int add = (M5.BtnB.isPressed()) ? -1 : 1;
    if (M5.BtnA.isHolding())
    {
      add = M5.BtnA.getClickCount() ? -1 : 1;
    }
    v += add;
    if (v <= 255)
    {
      M5.Speaker.setVolume(v);
    }
  }
}
