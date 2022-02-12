#include <SD.h>
#include <HTTPClient.h>
#include <math.h>

/// need ESP8266Audio library. ( URL : https://github.com/earlephilhower/ESP8266Audio/ )
#include <AudioOutput.h>
#include <AudioFileSourceSD.h>
#include <AudioFileSourceID3.h>
#include <AudioGeneratorMP3.h>

#include <M5UnitLCD.h>
#include <M5UnitOLED.h>
#include <M5Unified.h>

/// set M5Speaker virtual channel (0-7)
static constexpr uint8_t m5spk_virtual_channel = 0;

/// set your mp3 filename
static constexpr const char* filename[] =
{
  "/mp3/file01.mp3",
  "/mp3/file02.mp3",
  "/mp3/file03.mp3",
  "/mp3/file04.mp3",
};
static constexpr const size_t filecount = sizeof(filename) / sizeof(filename[0]);

class AudioOutputM5Speaker : public AudioOutput
{
  public:
    AudioOutputM5Speaker(m5::Speaker_Class* m5sound, uint8_t virtual_sound_channel = 0)
    {
      _m5sound = m5sound;
      _virtual_ch = virtual_sound_channel;
      _tri_buffer_index = 0;
      _tri_index = 0;
    }
    virtual ~AudioOutputM5Speaker(void) override {};
    virtual bool begin(void) override { return true; }
    virtual bool ConsumeSample(int16_t sample[2]) override
    {
      _tri_buffer[_tri_index][_tri_buffer_index  ] = sample[0];
      _tri_buffer[_tri_index][_tri_buffer_index+1] = sample[1];
      _tri_buffer_index += 2;

      if (_tri_buffer_index < flip_buf_size)
      {
        return true;
      }

      flush();
      return false;
    }
    virtual void flush(void) override
    {
      if (_tri_buffer_index)
      {
        /// If there is no room in the play queue, playRAW will return false, so repeat until true is returned.
        while (false == _m5sound->playRAW(_tri_buffer[_tri_index], _tri_buffer_index, hertz, true, 1, _virtual_ch)) { taskYIELD(); }
        _tri_index = _tri_index < 2 ? _tri_index + 1 : 0;
        _tri_buffer_index = 0;
      }
    }
    virtual bool stop(void) override
    {
      flush();
      _m5sound->stop(_virtual_ch);
      return true;
    }

    const int16_t* getBuffer(void) const { return _tri_buffer[(_tri_index + 2) % 3]; }

  protected:
    m5::Speaker_Class* _m5sound;
    uint8_t _virtual_ch;
    static constexpr size_t flip_buf_size = 1024;
    int16_t _tri_buffer[3][flip_buf_size];
    size_t _tri_buffer_index;
    size_t _tri_index = 0;
};


#define FFT_SIZE 512
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


static AudioFileSourceSD file;
static AudioOutputM5Speaker out(&M5.Speaker, m5spk_virtual_channel);
static AudioGeneratorMP3 mp3;
static AudioFileSourceID3* id3 = nullptr;
static fft_t fft;
static bool fft_enabled = false;
static uint16_t prev_y[(FFT_SIZE/2)+1];
static uint16_t peak_y[(FFT_SIZE/2)+1];
static int header_height = 0;
static size_t fileindex = 0;

void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string)
{
  (void)cbData;
  if (string[0] == 0) { return; }
  if (strcmp(type, "eof") == 0)
  {
    M5.Display.display();
    header_height = M5.Display.getCursorY();
    return;
  }
  M5.Display.printf("%s: ", type);
  M5.Display.println(string);
}

void stop(void)
{
  if (id3 == nullptr) return;
  out.stop();
  mp3.stop();
  id3->RegisterMetadataCB(nullptr, nullptr);
  id3->close();
  file.close();
  delete id3;
  id3 = nullptr;
}

void play(const char* fname)
{
  if (id3 != nullptr) { stop(); }
  M5.Display.setCursor(0, 12);
  file.open(fname);
  id3 = new AudioFileSourceID3(&file);
  id3->RegisterMetadataCB(MDCallback, (void*)"ID3TAG");
  id3->open(fname);
  mp3.begin(id3, &out);
}

void setup()
{
  auto cfg = M5.config();
  cfg.external_spk = true;
  M5.begin(cfg);
  M5.Display.setEpdMode(epd_mode_t::epd_fastest);
  if (M5.Display.width() < M5.Display.height())
  {
    M5.Display.setRotation(M5.Display.getRotation()^1);
  }
  fft_enabled = (!M5.Display.isEPD() && M5.Display.getPanel()->bus()->busType() != m5gfx::bus_type_t::bus_i2c);
  SD.begin(GPIO_NUM_4, SPI, 25000000);

  M5.Display.fillRect(0, 8, M5.Display.width(), 3, TFT_BLACK);
  M5.Display.setFont(&fonts::lgfxJapanGothic_8);

  header_height = 36;
  fft_enabled = !M5.Display.isEPD();
  for (int x = 0; x < (FFT_SIZE/2)+1; ++x)
  {
    prev_y[x] = INT16_MAX;
    peak_y[x] = INT16_MAX;
  }

  play(filename[fileindex]);
}

void loop()
{
  if (mp3.isRunning())
  {
    if (!mp3.loop()) { mp3.stop(); }
  }
  else
  {
    delay(1);
  }

  M5.update();
  if (M5.BtnA.wasClicked())
  {
    M5.Speaker.tone(440, 100);
    stop();
    M5.Display.fillRect(0, 8, M5.Display.width(), header_height - 8, TFT_BLACK);
    if (++fileindex >= filecount) { fileindex = 0; }
    play(filename[fileindex]);
  }
  else
  if (M5.BtnA.isHolding() || M5.BtnB.isPressed() || M5.BtnC.isPressed())
  {
    size_t v = M5.Speaker.getVolume();
    if (M5.BtnB.isPressed()) { --v; } else { ++v; }
    if (v <= 255 || M5.BtnA.isHolding())
    {
      M5.Speaker.setVolume(v);
    }
  }

  if (!M5.Display.displayBusy())
  { // draw volume bar
    static int px;
    uint8_t v = M5.Speaker.getVolume();
    int x = v * (M5.Display.width()) >> 8;
    if (px != x)
    {
      M5.Display.fillRect(x, 8, px - x, 3, px < x ? TFT_GREEN : TFT_BLACK);
      px = x;
    }
  }

  if (fft_enabled && !M5.Display.displayBusy())
  { // draw stereo level meter
    static int prev_x[2];

    auto data = out.getBuffer();
    uint16_t level[2] = { 0, 0 };
    for (int i = 0; i < FFT_SIZE >> 1; i += 16)
    {
      uint32_t lv = abs(data[i]);
      if (level[0] < lv) { level[0] = lv; }
      lv = abs(data[i+1]);
      if (level[1] < lv) { level[1] = lv; }
    }
    for (int i = 0; i < 2; ++i)
    {
      int x = (level[i] * M5.Display.width()) / INT16_MAX;
      int px = prev_x[i];
      if (px != x)
      {
        prev_x[i] = x;
        M5.Display.fillRect(x, i * 4, px - x, 3, px < x ? TFT_BLUE : TFT_BLACK);
      }
    }
  }

  if (fft_enabled && M5.Speaker.isPlaying(m5spk_virtual_channel) == 2)
  {
    static bool fft_executed = false;
    if (!fft_executed)
    {
      fft.exec(out.getBuffer());
      fft_executed = true;
    }

    if (M5.Speaker.isPlaying(m5spk_virtual_channel) == 2)
    { // draw fft level meter
      fft_executed = false;

      int dsp_height = M5.Display.height();
      int fft_height = dsp_height - header_height;

      M5.Display.startWrite();
      int xe = M5.Lcd.width() >> 2;
      if (xe > (FFT_SIZE/2)+1) { xe = (FFT_SIZE/2)+1; }
      for (int x = 0; x < xe; ++x)
      {
        int32_t f = fft.get(x) * fft_height;
        int y = dsp_height - std::min(fft_height, f >> 19);
        int py = prev_y[x];
        if (y != py)
        {
          M5.Lcd.fillRect(x*4, y, 3, py - y, (y < py) ? TFT_GREEN : TFT_BLACK);
          prev_y[x] = y;
        }
        py = peak_y[x] + 1;
        if (py < y)
        {
          M5.Lcd.writeFastHLine(x*4, py-1, 3, TFT_BLACK);
        }
        else
        {
          py = y - 1;
        }
        if (peak_y[x] != py)
        {
          peak_y[x] = py;
          M5.Lcd.writeFastHLine(x*4, py, 3, TFT_WHITE);
        }
      }
      M5.Display.endWrite();
    }
  }
}
