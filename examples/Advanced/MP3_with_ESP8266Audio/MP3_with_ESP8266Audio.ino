#include <SD.h>
#include <math.h>

/// need ESP8266Audio library. ( URL : https://github.com/earlephilhower/ESP8266Audio/ )
#include <AudioOutput.h>
#include <AudioFileSourceSD.h>
#include <AudioFileSourceID3.h>
#include <AudioGeneratorMP3.h>
#include <AudioGeneratorWAV.h>

#include <M5UnitLCD.h>
#include <M5UnitOLED.h>
#include <M5Unified.h>
#include <atomic>
#include <vector>
#include <algorithm>

// Shared with the other audio examples (same file in each folder).
#include "AudioOutputM5Speaker.h"
#include "AudioUI.h"

/// set M5Speaker virtual channel (0-7)
static constexpr uint8_t m5spk_virtual_channel = 0;

/// Every *.mp3 and *.wav in this folder of the card is played, in name order.
static constexpr const char* mp3_dir = "/mp3";
static std::vector<String> filename;

static AudioFileSourceSD file;
static AudioOutputM5Speaker out(&M5.Speaker, m5spk_virtual_channel);
static AudioGeneratorMP3 mp3;
static AudioGeneratorWAV wav;
static AudioGenerator* generator = nullptr; // &mp3 or &wav while a file is open
static AudioFileSourceID3* id3 = nullptr; // only for *.mp3
static AudioUI ui;
static size_t fileindex = 0; // file the decode task is on
// Decoding runs in its own task so that drawing (which can take longer than
// one audio buffer) never starves it. The UI asks for another file through
// this: requests add up (+1 next, -1 previous) until the decode task takes
// them, so two "next" while it is busy move two files on, and next then
// previous cancel out.
static std::atomic<int> track_request { 0 };

static void scanFiles(void)
{
  filename.clear();
  auto dir = SD.open(mp3_dir);
  if (!dir) { return; }
  for (auto f = dir.openNextFile(); f; f = dir.openNextFile())
  {
    bool is_dir = f.isDirectory();
    String name = f.name(); // a bare name or a full path, depending on the core version
    f.close();
    String lower = name;
    lower.toLowerCase();
    if (is_dir || !(lower.endsWith(".mp3") || lower.endsWith(".wav"))) { continue; }
    filename.push_back(name.startsWith("/") ? name : String(mp3_dir) + "/" + name);
  }
  dir.close();
  std::sort(filename.begin(), filename.end());
}

/// ID3 tags: title and artist go on the display, everything is logged.
static void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string)
{
  (void)cbData;
  (void)isUnicode;
  if (string[0] == 0 || strcmp(type, "eof") == 0) { return; }
  if (strcmp(type, "Title") == 0)          { ui.setMeta(1, string, type); }
  else if (strcmp(type, "Performer") == 0) { ui.setMeta(2, string, type); }
  else { M5_LOGI("%s: %s", type, string); }
}

static void stop(void)
{
  if (generator == nullptr) return;
  out.stop();
  generator->stop();
  generator = nullptr;
  if (id3 != nullptr)
  {
    id3->RegisterMetadataCB(nullptr, nullptr);
    id3->close();
    delete id3;
    id3 = nullptr;
  }
  file.close();
}

/// Start a file. false when it could not be opened or decoded (nothing is playing then).
static bool play(const char* fname)
{
  stop();
  ui.clearMeta(); // nothing from the previous file stays on screen
  const char* base = strrchr(fname, '/');
  ui.setMeta(0, base ? base + 1 : fname, "file");
  bool ok = file.open(fname);
  if (ok)
  {
    String lower = fname;
    lower.toLowerCase();
    if (lower.endsWith(".wav"))
    {
      generator = &wav;
      ok = generator->begin(&file, &out);
    }
    else
    {
      id3 = new AudioFileSourceID3(&file);
      id3->RegisterMetadataCB(MDCallback, nullptr);
      id3->open(fname);
      generator = &mp3;
      ok = generator->begin(id3, &out);
    }
  }
  if (!ok)
  {
    stop();
    ui.setMeta(1, "(cannot play)", "error");
  }
  return ok;
}

/// Move `step` files on (0: the current one) and play; a file that cannot
/// be played is skipped, going forwards. When none can be played, nothing
/// plays until the next button press.
static void playNext(int step)
{
  int n = filename.size();
  for (int tries = 0; tries < n; ++tries)
  {
    int index = ((int)fileindex + step) % n; // step may have accumulated beyond +-n
    if (index < 0) { index += n; }
    fileindex = index;
    if (play(filename[fileindex].c_str())) { return; }
    step = 1;
  }
}

static void decodeTask(void*)
{
  for (;;)
  {
    int step = track_request.exchange(0);
    if (step) { playNext(step); }
    if (generator != nullptr && generator->isRunning())
    {
      if (!generator->loop()) { playNext(1); } // end of file: on to the next one
    }
    else
    {
      M5.delay(1);
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
  M5.Log.setLogLevel(m5::log_target_serial, ESP_LOG_INFO); // file names and tags go to the serial log too

  { /// custom setting
    auto spk_cfg = M5.Speaker.config();
    /// Increasing the sample_rate will improve the sound quality instead of increasing the CPU load.
    spk_cfg.sample_rate = 96000; // default:64000 (64kHz)  e.g. 48000 , 50000 , 80000 , 96000 , 100000 , 128000 , 144000 , 192000 , 200000
    M5.Speaker.config(spk_cfg);
  }

  M5.Speaker.begin();

  ui.setup(&M5.Display, "MP3 player");
  out.setup();
  out.setMonitor([](void* arg, const int16_t* stereo, size_t frames) { ((AudioUI*)arg)->feed(stereo, frames); }, &ui);

  while (false == SD.begin(GPIO_NUM_4, SPI, 25000000))
  {
    M5.delay(500);
  }

  scanFiles();
  if (filename.empty())
  {
    ui.setMeta(1, String("no *.mp3 / *.wav in " + String(mp3_dir)).c_str());
    for (;;) { ui.loop(); M5.delay(10); } // keep drawing so the message shows (and scrolls on a short display)
  }
  playNext(0);
  if (pdPASS != xTaskCreatePinnedToCore(decodeTask, "decodeTask", 4096, nullptr, 2, nullptr, (SOC_CPU_CORES_NUM > 1) ? 1 : 0))
  {
    stop();
    ui.setMeta(1, "(cannot start the decode task)", "error");
    for (;;) { ui.loop(); M5.delay(10); }
  }
}

void loop(void)
{
  ui.loop();

  {
    static int prev_frame;
    int frame;
    do
    {
      M5.delay(1);
    } while (prev_frame == (frame = millis() >> 3)); /// 8 msec cycle wait
    prev_frame = frame;
  }

  M5.update();
  auto in = ui.update();
  if (in.track) { track_request += in.track; } // handled by the decode task
}
