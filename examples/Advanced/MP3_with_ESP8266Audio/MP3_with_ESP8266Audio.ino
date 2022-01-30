#include <SD.h>
#include <AudioOutput.h>
#include <AudioFileSourceSD.h>
#include <AudioFileSourceID3.h>
#include <AudioGeneratorMP3.h>
#include <M5UnitLCD.h>
#include <M5UnitOLED.h>
#include <M5Unified.h>

class AudioOutputM5Sound : public AudioOutput
{
  public:
    AudioOutputM5Sound(m5::Sound_Class* m5sound, uint8_t sound_channel = 0)
    {
      _m5sound = m5sound;
      _sound_channel = sound_channel;
      _flip_buffer_index = 0;
      _flip_index = 0;
    }
    virtual ~AudioOutputM5Sound(void) override {};
    virtual bool begin(void) override { return true; }
    virtual bool ConsumeSample(int16_t sample[2]) override
    {
      _flip_buffer[_flip_index][_flip_buffer_index  ] = sample[0];
      _flip_buffer[_flip_index][_flip_buffer_index+1] = sample[1];
      _flip_buffer_index += 2;
      if (_flip_buffer_index < flip_buf_size) { return true; }

      flush();
      return false;
    }
    virtual void flush(void) override
    {
      _m5sound->playRAW(_flip_buffer[_flip_index], _flip_buffer_index, true, hertz, 1, _sound_channel);
      _flip_index = _flip_index < 2 ? _flip_index + 1 : 0;
      _flip_buffer_index = 0;
    }
    virtual bool stop(void) override
    {
      _m5sound->stopPlay(_sound_channel);
      return true;
    }

  protected:
    m5::Sound_Class* _m5sound;
    uint8_t _sound_channel;
    static constexpr size_t flip_buf_size = 512;
    int16_t _flip_buffer[3][flip_buf_size];
    size_t _flip_buffer_index;
    size_t _flip_index = 0;
};

static constexpr const size_t filecount = 4;
static constexpr const char* filename[filecount] =
{
  "/mp3/file01.mp3",
  "/mp3/file02.mp3",
  "/mp3/file03.mp3",
  "/mp3/file04.mp3",
};
size_t fileindex = 0;
AudioFileSourceSD file;
AudioOutputM5Sound out(&M5.Sound);
AudioGeneratorMP3 mp3;
AudioFileSourceID3* id3 = nullptr;

void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string)
{
  (void)cbData;
  if (string[0] == 0) { return; }
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
  SD.begin(GPIO_NUM_4, SPI, 25000000);

  int v = M5.Sound.getVolume();
  int x = v * (M5.Display.width() - 4) / 255;
  M5.Display.startWrite();
  M5.Display.drawRect(0, 0, M5.Display.width(), 10, TFT_WHITE);
  M5.Display.fillRect(2, 2, x, 6, TFT_GREEN);
  M5.Display.fillRect(2 + x, 2, M5.Display.width() - (x + 4), 6, TFT_BLACK);
  M5.Display.endWrite();
  M5.Display.setFont(&fonts::efontJA_16);
 
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
    M5.Sound.tone(440, 100);
    stop();
    if (++fileindex >= filecount) { fileindex = 0; }
    M5.Display.fillRect(0, 10, M5.Display.width(), M5.Display.height(), TFT_BLACK);
    play(filename[fileindex]);
  }
  if (M5.BtnB.isPressed() || M5.BtnC.isPressed())
  {
    size_t v = M5.Sound.getVolume();
    if (M5.BtnB.isPressed()) { --v; }
    if (M5.BtnC.isPressed()) { ++v; }
    if (v <= 255)
    {
      M5.Sound.setVolume(v);
      int x = v * (M5.Display.width() - 4) / 255;
      M5.Display.startWrite();
      M5.Display.fillRect(2, 2, x, 6, TFT_GREEN);
      M5.Display.fillRect(2 + x, 2, M5.Display.width() - (x + 4), 6, TFT_BLACK);
      M5.Display.endWrite();
    }
  }
}
