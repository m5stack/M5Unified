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
    AudioOutputM5Speaker(m5::Speaker_Class* m5sound, uint8_t sound_channel = 0)
    {
      _m5sound = m5sound;
      _sound_channel = sound_channel;
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
        while (false == _m5sound->playRAW(_tri_buffer[_tri_index], _tri_buffer_index, hertz, true, 1, _sound_channel)) { taskYIELD(); }
        _tri_index = _tri_index < 2 ? _tri_index + 1 : 0;
        _tri_buffer_index = 0;
      }
    }
    virtual bool stop(void) override
    {
      flush();
      _m5sound->stop(_sound_channel);
      return true;
    }

    const int16_t* getBuffer(void) const { return _tri_buffer[(_tri_index + 2) % 3]; }

  protected:
    m5::Speaker_Class* _m5sound;
    uint8_t _sound_channel;
    static constexpr size_t flip_buf_size = 512;
    int16_t _tri_buffer[3][flip_buf_size];
    size_t _tri_buffer_index;
    size_t _tri_index = 0;
};

#define FFT_SIZE 512
struct fft_t
{
  float Wr[FFT_SIZE + 1];
  float Wi[FFT_SIZE + 1];
  float Fr[FFT_SIZE + 1];
  float Fi[FFT_SIZE + 1];
  int32_t br[FFT_SIZE + 1];
  uint16_t prev_y[(FFT_SIZE/2)+1] = { 0 };
  uint16_t peak_y[(FFT_SIZE/2)+1] = { 0 };

  size_t fftStMax;

  fft_t(void)
  {
  #ifndef M_PI
  #define M_PI 3.141592653
  #endif

    fftStMax = logf( (float)FFT_SIZE ) / log(2.0) + 0.5;
    static constexpr float omega = 2.0f * M_PI / FFT_SIZE;
    static constexpr int s4 = FFT_SIZE / 4;
    static constexpr int s2 = FFT_SIZE / 2;
    for ( int i = 1 ; i < s4 ; ++i)
    {
    float f = cosf( omega * i);
      Wi[s4 + i] = f;
      Wi[s4 - i] = f;
      Wr[     i] = f;
      Wr[s2 - i] = -f;
    }
    Wi[s4] = Wr[0] = 1;

    int loop = 1;
    br[0] = 0;
    br[1] = FFT_SIZE / 2;
    for ( int j = 0 ; j < fftStMax - 1 ; ++j )
    {
      br[ loop * 2 ] = br[ loop ] / 2;
      loop = loop * 2;
      for ( int i = 1 ; i < loop ; ++i )
      {
        br[loop + i] = br[loop] + br[i];
      }
    }
    for (int x = 0; x < (FFT_SIZE/2)+1; ++x)
    {
      prev_y[x] = INT16_MAX;
      peak_y[x] = INT16_MAX;
    }
  }

  void exec( const int16_t* xin)
  {
    memset(Fi, 0, sizeof(Fi));
    for ( int j = 0 ; j < FFT_SIZE/2 ; ++j )
    {
      float basej = 0.5*(1.0-Wr[j]);
      int r = FFT_SIZE - j - 1;
      Fr[br[j]] = basej * xin[ j ];
      Fr[br[r]] = basej * xin[ r ];
    }

    int _2_s = 1;
    int fftSt = 0;
    do
    {
      int _2_s_1 = _2_s;
      _2_s <<= 1;
      int fftSt2jMax = FFT_SIZE / _2_s;
      int fftSt2j = 0;
      do
      {
        int k = 0;
        do
        {
          int l = _2_s * fftSt2j + k;
          int m = _2_s_1 * (2 * fftSt2j + 1) + k;
          int p = fftSt2jMax * k;
          float Wxmr = Fr[m] * Wr[p] + Fi[m] * Wi[p];
          float Wxmi = Fi[m] * Wr[p] - Fr[m] * Wi[p];
          Fr[m] = Fr[l] - Wxmr;
          Fi[m] = Fi[l] - Wxmi;
          Fr[l] += Wxmr;
          Fi[l] += Wxmi;
        } while ( ++k < _2_s_1) ;
      } while ( ++fftSt2j < fftSt2jMax );
    } while ( ++fftSt < fftStMax );
  }

  void draw(const int16_t* inbuf)
  {
    int height = M5.Display.height();

    exec(inbuf);

    M5.Lcd.startWrite();
    int xe = M5.Lcd.width() >> 2;
    if (xe > (FFT_SIZE/2)+1) { xe = (FFT_SIZE/2)+1; }
    for (int x = 0; x < xe; ++x)
    {
      int32_t f = sqrtf(Fr[ x ] * Fr[ x ] + Fi[ x ] * Fi[ x ]);
      int y = height - std::min(height - 80, f >> 12);
      int py = prev_y[x];
      if (y != py)
      {
        M5.Lcd.fillRect(x*4, y, 3, py - y, (y < py) ? TFT_BLUE : TFT_BLACK);
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
    M5.Lcd.endWrite();
  }
};

static AudioFileSourceSD file;
static AudioOutputM5Speaker out(&M5.Speaker);
static AudioGeneratorMP3 mp3;
static AudioFileSourceID3* id3 = nullptr;
static fft_t fft;
static bool fft_enabled = false;
static size_t fileindex = 0;

void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string)
{
  (void)cbData;
  if (string[0] == 0) { return; }
  M5.Display.printf("%s: ", type);
  M5.Display.println(string);
  if (strcmp(type, "eof") == 0) { M5.Display.display(); }
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
  M5.Display.setCursor(0, 10);
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
  if (M5.Display.width() > M5.Display.height())
  {
    M5.Display.setRotation(M5.Display.getRotation()^1);
  }
  fft_enabled = (!M5.Display.isEPD() && M5.Display.getPanel()->bus()->busType() != m5gfx::bus_type_t::bus_i2c);
  SD.begin(GPIO_NUM_4, SPI, 25000000);

  int v = M5.Speaker.getVolume();
  int x = v * (M5.Display.width() - 4) / 255;
  M5.Display.startWrite();
  M5.Display.drawRect(0, 0, M5.Display.width(), 10, TFT_WHITE);
  M5.Display.fillRect(2, 2, x, 6, TFT_GREEN);
  M5.Display.fillRect(2 + x, 2, M5.Display.width() - (x + 4), 6, TFT_BLACK);
  M5.Display.endWrite();
  M5.Display.setFont(&fonts::lgfxJapanGothic_12);

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
    M5.Display.fillRect(0, 10, M5.Display.width(), 80, TFT_BLACK);
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
  {
    static int px;
    uint8_t v = M5.Speaker.getVolume();
    int x = 2 + v * (M5.Display.width() - 4) / 255;
    if (px != x)
    {
      M5.Display.fillRect(x, 2, px - x, 6, px < x ? TFT_GREEN : TFT_BLACK);
      px = x;
    }
    if (fft_enabled && M5.Speaker.isPlaying(0) == 2)
    {
      fft.draw(out.getBuffer());
    }
  }
}
