// Fill in here, or pass both from the build (-DWIFI_SSID=\"...\" -DWIFI_PASS=\"...\").
#ifndef WIFI_SSID
#define WIFI_SSID "SET YOUR WIFI SSID"
#endif
#ifndef WIFI_PASS
#define WIFI_PASS "SET YOUR WIFI PASS"
#endif


#include <WiFi.h>
#include <HTTPClient.h>
#include <math.h>

/// need ESP8266Audio library. ( URL : https://github.com/earlephilhower/ESP8266Audio/ )
#include <AudioOutput.h>
#include <AudioFileSourceICYStream.h>
#include <AudioFileSource.h>
#include <AudioFileSourceBuffer.h>
#include <AudioGeneratorMP3.h>

#include <M5UnitLCD.h>
#include <M5UnitOLED.h>
#include <M5Unified.h>
#include <atomic>

// Shared with the other audio examples (same file in each folder).
#include "AudioOutputM5Speaker.h"
#include "AudioUI.h"

// Some cores (ESP32-P4) do not name the second CPU; the speaker task goes there when there is one.
#ifndef APP_CPU_NUM
#define APP_CPU_NUM ((SOC_CPU_CORES_NUM > 1) ? 1 : 0)
#endif

/// set M5Speaker virtual channel (0-7)
static constexpr uint8_t m5spk_virtual_channel = 0;

/// set web radio station url (plain http MP3 streams; a station that cannot
/// be opened is skipped automatically)
static constexpr const char* station_list[][2] =
{
  {"Radio Paradise"    , "http://stream.radioparadise.com/mp3-128"},
  {"FIP"               , "http://icecast.radiofrance.fr/fip-midfi.mp3"},
  {"FIP Jazz"          , "http://icecast.radiofrance.fr/fipjazz-midfi.mp3"},
  {"France Musique"    , "http://icecast.radiofrance.fr/francemusique-midfi.mp3"},
  {"181.fm Beatles"    , "http://listen.181fm.com/181-beatles_128k.mp3"},
  {"181.fm Classical"  , "http://listen.181fm.com/181-classical_128k.mp3"},
  {"181.fm Jazz Mix"   , "http://listen.181fm.com/181-jazzmix_128k.mp3"},
  {"KEXP"              , "http://kexp-mp3-128.streamguys1.com/kexp128.mp3"},
  {"WQXR"              , "http://stream.wqxr.org/wqxr"},
  {"BBC World Service" , "http://stream.live.vc.bbcmedia.co.uk/bbc_world_service"},
  {"Classic FM"        , "http://media-ice.musicradio.com:80/ClassicFMMP3"},
  {"Lite Favorites"    , "http://naxos.cdnstream.com:80/1255_128"},
};
static constexpr const size_t stations = sizeof(station_list) / sizeof(station_list[0]);

// Stream buffer: about two seconds of a 128 kbps stream. Playback starts
// only once it is mostly full and pauses to refill when it runs low, so a
// network hiccup costs one gap instead of a burst of dropouts.
static constexpr const int preallocateBufferSize = 32 * 1024;
static constexpr const int bufferStartLevel = preallocateBufferSize * 3 / 4;
static constexpr const int bufferLowLevel = preallocateBufferSize / 8;
static constexpr const uint32_t bufferWaitMs = 3000;
static constexpr const int preallocateCodecSize = 29192; // MP3 codec max mem needed
static void* preallocateBuffer = nullptr;
static void* preallocateCodec = nullptr;
static AudioOutputM5Speaker out(&M5.Speaker, m5spk_virtual_channel);
static AudioGenerator *decoder = nullptr;
static AudioFileSourceICYStream *file = nullptr;
static AudioFileSourceBuffer *buff = nullptr;
static AudioUI ui;
// Station numbers are shared between the UI and decode tasks, so use std::atomic.
static std::atomic<size_t> playing_index { 0 }; // station the decode task is on; the UI navigates relative to it
static std::atomic<size_t> playindex { ~0u }; // station requested (absolute); ~0u = none
// Station steps from the UI (+1 next, -1 previous). They add up until the
// decode task takes them, so two "next" while it is busy move two stations on.
static std::atomic<int> station_request { 0 };

static void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string)
{
  (void)cbData;
  if (strcmp(type, "StreamTitle") == 0)
  {
    ui.setMeta(1, string, type);
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

/// A station has been asked for (by the UI or the decode task itself).
static bool selected(void)
{
  return playindex != ~0u || station_request != 0;
}

/// Ask for the station after `index` from the decode task - unless a
/// selection made from the UI is already waiting, which wins.
static void playNext(size_t index)
{
  if (++index >= stations) { index = 0; }
  size_t none = ~0u;
  playindex.compare_exchange_strong(none, index);
}

/// Sleep for `ms`, cut short when a station gets selected.
static void waitUnlessSelected(uint32_t ms)
{
  uint32_t start = millis();
  while (!selected() && millis() - start < ms) { M5.delay(10); }
}

/// Keep filling the stream buffer until it holds `level` bytes. Returns false
/// when a station was selected meanwhile; sets *timed_out when the stream
/// could not deliver within bufferWaitMs (play on with what there is).
static bool waitForBuffer(int level, bool* timed_out)
{
  *timed_out = false;
  uint32_t start = millis();
  while (buff->getFillLevel() < (uint32_t)level)
  {
    if (selected()) { return false; }
    if (millis() - start >= bufferWaitMs) { *timed_out = true; break; }
    buff->loop();
    M5.delay(1);
  }
  return true;
}

/// A station is counted as working only once this much audio has actually
/// been played (buffering time does not count); one that fails earlier is
/// skipped like one that could not be opened.
static constexpr uint32_t stationOkSeconds = 2;

static void decodeTask(void*)
{
  size_t failures = 0;    // stations that failed in a row
  bool starved = false;   // the stream fell behind: play on, do not wait again until it caught up
  bool confirmed = false; // the current station has played for stationOkSeconds
  uint32_t frames_at_start = 0;
  for (;;)
  {
    M5.delay(1);
    int step = station_request.exchange(0);
    if (step)
    { // a UI request: relative to the station being played, and it wins over a pending automatic one
      int index = ((int)playing_index + step) % (int)stations;
      if (index < 0) { index += stations; }
      playindex = index;
    }
    if (playindex != ~0u)
    {
      auto index = playindex.exchange(~0u);
      if (index >= stations) { index = 0; }
      stop();
      playing_index = index;
      starved = false;
      confirmed = false;
      ui.clearMeta(); // nothing from the previous station stays on screen
      ui.setMeta(0, station_list[index][0], "station");
      file = new AudioFileSourceICYStream(station_list[index][1]);
      bool ok = file->isOpen();
      if (ok)
      {
        file->RegisterMetadataCB(MDCallback, (void*)"ICY");
        buff = new AudioFileSourceBuffer(file, preallocateBuffer, preallocateBufferSize);
        // The buffer's very first read fills it wholesale (replacing, not
        // adding to, anything loop() gathered before), so trigger that first,
        // then let loop() top it up before decoding starts.
        uint8_t dummy;
        buff->read(&dummy, 0);
        bool timed_out;
        if (!waitForBuffer(bufferStartLevel, &timed_out))
        { // a station was asked for. Should the request have cancelled out by the time it is taken,
          // this half-opened station is restarted rather than left without a decoder.
          size_t none = ~0u;
          playindex.compare_exchange_strong(none, index);
          continue;
        }
        starved = timed_out; // a slow stream: do not wait again until it catches up
        decoder = new AudioGeneratorMP3(preallocateCodec, preallocateCodecSize);
        ok = decoder->begin(buff, &out); // false if the stream broke meanwhile
      }
      if (ok)
      {
        frames_at_start = out.getFrames();
        continue;
      }
    }
    else if (decoder && decoder->isRunning())
    {
      if (!confirmed && out.getRate() != 0
       && out.getFrames() - frames_at_start >= out.getRate() * stationOkSeconds)
      {
        confirmed = true;
        failures = 0;
      }
      uint32_t level = buff->getFillLevel();
      if (starved)
      {
        if (level >= (uint32_t)bufferStartLevel) { starved = false; }
      }
      else if (level < (uint32_t)bufferLowLevel)
      { // running dry: refill before decoding on.
        bool timed_out;
        if (!waitForBuffer(bufferStartLevel, &timed_out)) { continue; }
        starved = timed_out;
      }
      if (decoder->loop()) { continue; }
      decoder->stop();
      if (confirmed)
      { // the stream ended or broke after playing: move on to the next station.
        playNext(playing_index);
        continue;
      }
    }
    else
    {
      continue;
    }
    // A station that could not be opened, broke during buffering, or stopped
    // before stationOkSeconds of audio came out: skip it. Once every station failed in a
    // row, wait a while before going round again. A selection from the UI
    // cuts the wait short.
    ui.setMeta(1, "(unavailable, skipping)");
    stop();
    if (++failures >= stations)
    {
      failures = 0;
      waitUnlessSelected(5000);
    }
    else
    {
      waitUnlessSelected(500);
    }
    playNext(playing_index);
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
  M5.Log.setLogLevel(m5::log_target_serial, ESP_LOG_INFO); // station and title changes go to the serial log too

  preallocateBuffer = malloc(preallocateBufferSize);
  preallocateCodec = malloc(preallocateCodecSize);
  if (!preallocateBuffer || !preallocateCodec) {
    M5.Display.printf("FATAL ERROR:  Unable to preallocate %d bytes for app\n", preallocateBufferSize + preallocateCodecSize);
    for (;;) { M5.delay(1000); }
  }

  { /// custom setting
    auto spk_cfg = M5.Speaker.config();
    /// Increasing the sample_rate will improve the sound quality instead of increasing the CPU load.
    spk_cfg.sample_rate = 48000; // default:64000 (64kHz)  e.g. 48000 , 50000 , 80000 , 96000 , 100000 , 128000 , 144000 , 192000 , 200000
    spk_cfg.task_pinned_core = APP_CPU_NUM;
    M5.Speaker.config(spk_cfg);
  }


  M5.Speaker.begin();

  M5.Display.println("Connecting to WiFi");
  WiFi.mode(WIFI_STA); // first: on ESP-Hosted boards (Tab5) the radio is only brought up here
  WiFi.disconnect();
  // Modem sleep makes the station receive only at DTIM intervals; a stream
  // then arrives in bursts and the buffer drains between them.
  WiFi.setSleep(false);

#if defined ( WIFI_SSID ) &&  defined ( WIFI_PASS )
  WiFi.begin(WIFI_SSID, WIFI_PASS);
#else
  WiFi.begin();
#endif

  // Try forever
  while (WiFi.status() != WL_CONNECTED) {
    M5.Display.print(".");
    M5.delay(100);
  }
  M5.Display.clear();

  ui.setup(&M5.Display, "WebRadio player");
  out.setup();
  out.setMonitor([](void* arg, const int16_t* stereo, size_t frames) { ((AudioUI*)arg)->feed(stereo, frames); }, &ui);

  playindex = 0;

  xTaskCreatePinnedToCore(decodeTask, "decodeTask", 4096, nullptr, 1, nullptr, PRO_CPU_NUM);
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
  if (in.track) { station_request += in.track; } // handled by the decode task
}
