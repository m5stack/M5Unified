// AudioUI.h
//
// Display and button handling shared by the audio examples: header text
// lines (song / station / device information, also written to the log),
// stereo level meter, FFT bars, waveform and a volume bar.
//
// An Arduino sketch can only include files from its own folder, so this file
// is copied into each example that uses it. Keep the copies identical.
#pragma once

#include <M5Unified.h>
#include <math.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

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

class AudioUI
{
public:
  static constexpr size_t meta_num = 3;    // text lines in the header
  static constexpr size_t meta_size = 128; // bytes per line

  /// Call after M5.begin(). `title` is the first header line until
  /// setMeta(0, ...) replaces it, and comes back with clearMeta().
  void setup(LGFX_Device* gfx, const char* title)
  {
    _gfx = gfx;
    _mutex = xSemaphoreCreateMutex();
    strncpy(_title, title ? title : "", sizeof(_title) - 1);
    for (size_t i = 0; i < meta_num; ++i) { _meta[i][0] = 0; }
    strncpy(_meta[0], _title, meta_size - 1);
    _meta_bits = (1 << meta_num) - 1;

    if (gfx == nullptr) { return; }
    if (gfx->width() < gfx->height())
    {
      gfx->setRotation(gfx->getRotation()^1);
    }
    gfx->setFont(&fonts::lgfxJapanGothic_12);
    gfx->setEpdMode(epd_mode_t::epd_fastest);
    gfx->setTextWrap(false);
    gfx->fillRect(0, 6, gfx->width(), 2, TFT_BLACK);

    _header_height = (gfx->height() > 80) ? 45 : 21;
    // The level peak moves one pixel per draw across the width, the FFT peak one pixel down the height.
    _silent_draw_count = ((gfx->width() > gfx->height()) ? gfx->width() : gfx->height()) + 1;
    _fft_enabled = !gfx->isEPD();
    if (_fft_enabled)
    {
      _wave_enabled = (gfx->getBoard() != m5gfx::board_M5UnitLCD);

      for (int y = _header_height; y < gfx->height(); ++y)
      {
        gfx->drawFastHLine(0, y, gfx->width(), bgcolor(y));
      }
    }

    // The waveform shows up to wave_samples samples: on a wide display each
    // sample is drawn _scale pixels wide, on a narrow one only the samples
    // that fit are shown (around the trigger edge).
    _scale = (gfx->width() + wave_samples - 1) / wave_samples;
    if (_scale < 1) { _scale = 1; }
    for (int x = 0; x < (FFT_SIZE/2)+1; ++x)
    {
      _prev_y[x] = INT16_MAX;
      _peak_y[x] = INT16_MAX;
    }
    for (size_t x = 0; x < wave_samples; ++x)
    {
      _wave_y[x] = gfx->height();
      _wave_h[x] = 0;
    }
  }

  /// Set header line `id` (0 .. meta_num-1). May be called from any task.
  /// An unchanged text is ignored; a changed one is written to the log as
  /// "label: text" (or just the text). An empty text clears the line.
  void setMeta(size_t id, const char* text, const char* label = nullptr)
  {
    if (id >= meta_num || _mutex == nullptr) { return; }
    if (text == nullptr) { text = ""; }
    char line[meta_size];
    strncpy(line, text, meta_size - 1);
    line[meta_size - 1] = 0;
    for (char* c = line; *c; ++c) { if ((uint8_t)*c < 0x20) { *c = ' '; } } // a newline would reset the scroller's cursor
    bool changed = false;
    xSemaphoreTake(_mutex, portMAX_DELAY);
    if (strcmp(_meta[id], line) != 0)
    {
      memcpy(_meta[id], line, meta_size);
      _meta_bits |= 1 << id;
      changed = true;
    }
    xSemaphoreGive(_mutex);
    if (changed && text[0])
    {
      if (label) { M5_LOGI("%s: %s", label, text); }
      else       { M5_LOGI("%s", text); }
    }
  }

  /// Back to the title alone: call on a track change or a disconnect so that
  /// text from the previous track does not stay on screen.
  void clearMeta(void)
  {
    if (_mutex == nullptr) { return; }
    xSemaphoreTake(_mutex, portMAX_DELAY);
    strncpy(_meta[0], _title, meta_size - 1);
    for (size_t i = 1; i < meta_num; ++i) { _meta[i][0] = 0; }
    _meta_bits = (1 << meta_num) - 1;
    xSemaphoreGive(_mutex);
  }

  /// Give the display the audio just queued: `frames` stereo frames from
  /// `stereo` (raw_frames or more lets the waveform lock onto a rising
  /// edge; nullptr or 0 means silence). May be called from the producer
  /// task; the copy is short and made under a mutex, so the producer may
  /// reuse its buffer as soon as this returns.
  void feed(const int16_t* stereo, size_t frames)
  {
    if (_mutex == nullptr) { return; }
    if (stereo == nullptr) { frames = 0; }
    if (frames > raw_frames) { frames = raw_frames; }
    xSemaphoreTake(_mutex, portMAX_DELAY);
    if (frames) { memcpy(_feed_data, stereo, frames * 2 * sizeof(int16_t)); }
    _feed_frames = frames;
    _feed_fresh = true;
    xSemaphoreGive(_mutex);
  }

  /// Draw: header text, volume bar and, when feed() brought new audio since
  /// the last call, the meters and waveform.
  void loop(void)
  {
    auto gfx = _gfx;
    if (gfx == nullptr) { return; }

    drawMeta(gfx);

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

    if (_fft_enabled && !gfx->displayBusy() && _mutex != nullptr)
    {
      // Take the latest audio as one consistent copy.
      size_t frames = 0;
      bool fresh;
      xSemaphoreTake(_mutex, portMAX_DELAY);
      fresh = _feed_fresh;
      if (fresh)
      {
        _feed_fresh = false;
        frames = _feed_frames;
        if (frames) { memcpy(_raw_data, _feed_data, frames * 2 * sizeof(int16_t)); }
      }
      xSemaphoreGive(_mutex);
      if (fresh)
      { // silence (0 frames) is shown as zeros for a while so that the peak marks fall
        _silent_draws = frames ? 0 : _silent_draw_count;
      }
      if (frames || _silent_draws)
      {
        if (frames == 0) { --_silent_draws; }
        if (frames < raw_frames) { memset(&_raw_data[frames * 2], 0, (raw_frames - frames) * 2 * sizeof(int16_t)); }
        drawMeters(gfx, frames ? frames : raw_frames);
      }
    }
  }

  struct input_t
  {
    int track = 0;        // +1: next track, -1: previous track
    bool volume = false;  // the speaker volume was changed
  };

  /// Call after M5.update(). BtnA: click = next, double click = previous,
  /// hold = volume up (down when a click came first). BtnB / BtnC: volume
  /// down / up. A touch outside the virtual buttons acts like BtnA.
  /// Volume changes are applied to M5.Speaker.
  input_t update(void)
  {
    input_t res;
    const auto td = M5.Touch.getDetail();
    const uint32_t now = millis();
    // Known limitations: sliding onto the strip and 1px button-boundary mismatches are not reconciled.
    // A previous touch's button-release debounce is not distinguished from the current contact.
    if (td.wasPressed())
    { // New contact -> classify by its starting position, retained through release.
      if (!touchOnButtons(&_touch_button)) { _touch_button = 3; }
      _touch_hold = 0;
      if (_touch_button == 3) { M5.Speaker.tone(440, 50); }
    }
    if (M5.BtnA.wasPressed()) { M5.Speaker.tone(440, 50); }
    const bool outside = _touch_button == 3;
    const int clicks = M5.BtnA.wasClicked() + (outside && td.wasClicked() && !_touch_hold);
    if (clicks)
    { // Release -> append to the shared sequence; three or more always means ignore.
      _clicks = _clicks + clicks < 3 ? _clicks + clicks : 3;
      _click_ms = now;
    }
    if (!td.isPressed()) { _touch_hold = 0; } // Released touch -> idle.
    if (!M5.BtnA.isPressed()) { _a_hold = 0; } // Released button -> idle.
    const bool touch_begin = outside && td.isHolding() && !_touch_hold;
    const bool a_begin = M5.BtnA.wasHold() && !_a_hold;
    if (touch_begin || a_begin)
    { // First hold (including drag) -> latch direction and consume the click sequence.
      const int8_t direction = _clicks ? -1 : 1;
      if (touch_begin) { _touch_hold = direction; }
      if (a_begin) { _a_hold = direction; }
      _clicks = 0;
    }
    const bool touch_active = td.isPressed() && (outside || _touch_button == 0);
    if (_clicks && !touch_active && !M5.BtnA.isPressed() && now - _click_ms >= 500)
    { // Idle timeout -> decide once and empty the sequence.
      if (_clicks < 3)
      {
        res.track = _clicks == 1 ? 1 : -1;
        M5.Speaker.tone(_clicks == 1 ? 1000 : 800, 100);
      }
      _clicks = 0;
    }
    const int hold = _touch_hold ? _touch_hold : _a_hold;
    const int add = hold - M5.BtnB.isPressed() + M5.BtnC.isPressed();
    const int volume = M5.Speaker.getVolume() + add;
    if (add && volume >= 0 && volume <= 255)
    {
      M5.Speaker.setVolume(volume);
      res.volume = true;
    }
    return res;
  }

protected:
  LGFX_Device* _gfx = nullptr;
  SemaphoreHandle_t _mutex = nullptr;
  char _title[meta_size] = { 0 };
  char _meta[meta_num][meta_size];
  uint8_t _meta_bits = 0;
  int _header_height = 0;
  bool _fft_enabled = false;
  bool _wave_enabled = false;
  uint8_t _touch_button = 3; // contact origin: 0/1/2 = strip A/B/C, 3 = outside
  int8_t _touch_hold = 0;    // 0 = idle, -1/+1 = direction latched until touch release
  int8_t _a_hold = 0;        // 0 = idle, -1/+1 = direction latched until BtnA release
  uint8_t _clicks = 0;       // shared click sequence, saturating at 3 (ignore)
  uint32_t _click_ms = 0;    // last click in the sequence
  static constexpr size_t wave_samples = 320; // samples across the waveform
  static constexpr size_t raw_frames = 512;   // frames kept for the display: the waveform plus room to find its trigger edge
  static constexpr size_t edge_span = 4;      // frames the rise is measured over
  static constexpr size_t edge_near = 8;      // how far the trigger may drift and still count as the same edge
  static constexpr int32_t edge_zero_weight = 1; // how much a rise is penalized per unit of its level at the middle
  size_t _last_edge = 0;
  size_t _scale = 1;                          // pixels per waveform sample
  fft_t _fft;
  uint16_t _prev_y[(FFT_SIZE / 2)+1];
  uint16_t _peak_y[(FFT_SIZE / 2)+1];
  int16_t _wave_y[wave_samples];
  int16_t _wave_h[wave_samples];
  int16_t _raw_data[raw_frames * 2];   // what the meters are drawn from (display task only)
  int16_t _feed_data[raw_frames * 2];  // handed over by feed(), guarded by _mutex
  size_t _feed_frames = 0;
  bool _feed_fresh = false;
  uint16_t _silent_draw_count = 0; // draws needed for a full-scale peak mark to fall to zero (set in setup)
  uint16_t _silent_draws = 0;

  /// Whether the touch that has just begun is on the virtual button strip,
  /// and which button (0 = A, 1 = B, 2 = C): the same mapping M5.update()
  /// uses (raw panel coordinates, the strip is the bottom of the panel in its
  /// native orientation, height as configured, split in three across).
  bool touchOnButtons(uint8_t* button) const
  {
    if (!_gfx || !_gfx->getPanel()) { return false; }
    const auto& cfg = _gfx->getPanel()->config();
    if (cfg.panel_width <= 0 || cfg.panel_height <= 0) { return false; }
    // The strip may lie below the panel (CoreS3: y 240..279 on a 240 px panel), so a height of 0 still means y >= panel_height.
    const auto raw = M5.Touch.getTouchPointRaw(0);
    if (raw.y < (int)cfg.panel_height - (int)M5.getTouchButtonHeight()) { return false; }
    const int k = 65536 * 3 / cfg.panel_width; // the same fixed-point split as M5.update
    const int b = (raw.x * k) >> 16;
    *button = b < 0 ? 0 : b > 2 ? 2 : b;
    return true;
  }

  uint32_t bgcolor(int y) const
  {
    int h = _gfx->height();
    int dh = h - _header_height;
    int v = ((h - y) * 32) / dh; // multiply, not shift: y can reach h, and a negative left shift is undefined
    if (dh > 44)
    {
      int v2 = ((h - y - 1) * 32) / dh;
      if ((v >> 2) != (v2 >> 2))
      {
        return 0x666666u;
      }
    }
    return _gfx->color888(v + 2, v, v + 6);
  }

  void drawMeta(LGFX_Device* gfx)
  {
    if (_mutex == nullptr) { return; }
    // Work on a copy: the lines may be replaced from another task meanwhile.
    char meta[meta_num][meta_size];
    xSemaphoreTake(_mutex, portMAX_DELAY);
    uint8_t bits = _meta_bits;
    _meta_bits = 0;
    if (bits) { memcpy(meta, _meta, sizeof(meta)); }
    xSemaphoreGive(_mutex);

    if (_header_height > 32)
    { // one line per text
      if (bits)
      {
        gfx->startWrite();
        for (size_t id = 0; id < meta_num; ++id)
        {
          if (0 == (bits & (1<<id))) { continue; }
          size_t y = id * 12;
          if (y+12 >= _header_height) { continue; }
          gfx->setCursor(4, 8 + y);
          gfx->fillRect(0, 8 + y, gfx->width(), 12, gfx->getBaseColor());
          gfx->print(meta[id]);
          gfx->print(" "); // Garbage data removal when UTF8 characters are broken in the middle.
        }
        gfx->display();
        gfx->endWrite();
      }
      return;
    }

    // A short display: all texts scroll through one line.
    static int title_x;
    static int title_id;
    static int wait = INT16_MAX;
    static char scroll[meta_num][meta_size];
    if (bits)
    { // show the changed line at once, starting from the left: the highest changed line that has text
      // (an error or a title set right after the first line), else the first line
      memcpy(scroll, meta, sizeof(scroll));
      title_x = 4;
      title_id = 0;
      for (size_t id = meta_num; id-- > 0;)
      {
        if ((bits & (1 << id)) && scroll[id][0]) { title_id = id; break; }
      }
      gfx->fillRect(0, 8, gfx->width(), 12, gfx->getBaseColor());
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
        const char* text = scroll[tid];
        if (text[0] != 0)
        {
          gfx->print(text);
          gfx->print("  /  ");
          if (gfx->getCursorX() <= tx) { break; } // no progress (nothing drawable): never loop on it
          tx = gfx->getCursorX();
          if (++tid == meta_num) { tid = 0; }
          if (tx <= 4)
          {
            title_x = tx;
            title_id = tid;
          }
        }
        else
        {
          if ((no_data_bits |= 1 << tid) == ((1 << meta_num) - 1))
          {
            break;
          }
          if (++tid == meta_num) { tid = 0; }
        }
      } while (tx < gfx->width());
      --title_x;
      gfx->display();
      gfx->endWrite();
    }
  }

  /// Mono value (L+R) of frame `i` of the copied audio.
  int32_t mono(size_t i) const { return (int32_t)_raw_data[i * 2] + _raw_data[i * 2 + 1]; }

  /// Frame to put in the middle of the waveform: the steepest rise (over
  /// edge_span frames) in [offset, offset + length), rises through zero
  /// scoring higher than equally steep ones away from it. A rise near the
  /// one used last time is taken when it scores nearly as well, so the
  /// picture does not jump between similar edges of one period.
  size_t searchEdge(size_t offset, size_t length)
  {
    int32_t best = INT32_MIN, near_best = INT32_MIN;
    size_t best_i = offset, near_i = offset;
    for (size_t i = offset; i < offset + length; ++i)
    {
      int32_t d = mono(i + edge_span / 2) - mono(i - edge_span / 2) - abs(mono(i)) * edge_zero_weight;
      if (d > best) { best = d; best_i = i; }
      if (i + edge_near >= _last_edge && i <= _last_edge + edge_near && d > near_best) { near_best = d; near_i = i; }
    }
    size_t pick = best_i;
    if (best > 0 && near_best > 0 && near_best >= best - best / 4) { pick = near_i; }
    _last_edge = pick;
    return pick;
  }

  void drawMeters(LGFX_Device* gfx, size_t frames)
  {
    static int prev_x[2];
    static int peak_x[2];
    auto raw_data = _raw_data;
    gfx->startWrite();

    // draw stereo level meter
    for (size_t i = 0; i < 2; ++i)
    {
      int32_t level = 0;
      for (size_t j = i; j < frames * 2; j += 32)
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
    _fft.exec(raw_data);
    size_t bw = gfx->width() / 60;
    if (bw < 3) { bw = 3; }
    int32_t dsp_height = gfx->height();
    int32_t fft_height = dsp_height - _header_height - 1;
    size_t xe = gfx->width() / bw;
    if (xe > (FFT_SIZE/2)) { xe = (FFT_SIZE/2); }

    uint32_t bar_color[2] = { 0x000033u, 0x99AAFFu };

    for (size_t bx = 0; bx <= xe; ++bx)
    {
      size_t x = bx * bw;
      if ((x & 7) == 0) { gfx->display(); taskYIELD(); }
      int32_t f = _fft.get(bx);
      int32_t y = (int32_t)(((int64_t)f * fft_height) >> 18); // f can reach ~4M: keep the product in 64 bits
      if (y > fft_height) { y = fft_height; }
      y = dsp_height - y;
      int32_t py = _prev_y[bx];
      if (y != py)
      {
        gfx->fillRect(x, y, bw - 1, py - y, bar_color[(y < py)]);
        _prev_y[bx] = y;
      }
      py = _peak_y[bx] + 1;
      if (py < y)
      {
        gfx->writeFastHLine(x, py - 1, bw - 1, bgcolor(py - 1));
      }
      else
      {
        py = y - 1;
      }
      if (_peak_y[bx] != py)
      {
        _peak_y[bx] = py;
        gfx->writeFastHLine(x, py, bw - 1, TFT_WHITE);
      }
    }

    size_t width = gfx->width();
    size_t visible = width / _scale; // samples that fit on the display
    if (visible > wave_samples) { visible = wave_samples; }
    if (_wave_enabled && visible >= edge_span)
    {
      // Which frame the waveform starts at: the trigger edge lands in the
      // middle of the visible samples.
      size_t start = 0;
      if (frames > visible)
      {
        size_t half = visible / 2;
        start = searchEdge(half, frames - visible) - half;
      }
      int32_t mid = (_header_height + dsp_height) >> 1;
      int32_t wave_next = mid + (((256 - mono(start)) * fft_height) >> 17);
      for (size_t si = 0; si < visible; ++si)
      {
        size_t x0 = si * _scale;
        size_t w = _scale;
        if (x0 + w > width) { w = width - x0; }
        int32_t y = _wave_y[si];
        int32_t h = _wave_h[si];
        if (h > 0)
        { /// erase previous wave: restore the FFT bars / background under it.
          for (size_t xi = x0; xi < x0 + w; ++xi)
          {
            size_t bx = xi / bw;
            bool gap = (xi % bw) == bw - 1; // the 1 px space between bars
            int32_t yy = y;
            gfx->setAddrWindow(xi, yy, 1, h);
            do
            {
              uint32_t bg = (gap || yy < _peak_y[bx]) ? bgcolor(yy)
                          : (yy == _peak_y[bx]) ? 0xFFFFFFu
                          : bar_color[(yy >= _prev_y[bx])];
              gfx->writeColor(bg, 1);
            } while (++yy < y + h);
          }
        }
        size_t next = start + si + 1;
        if (next >= frames) { next = frames - 1; }
        int32_t y1 = wave_next;
        wave_next = mid + (((256 - mono(next)) * fft_height) >> 17);
        int32_t y2 = wave_next;
        if (y1 > y2)
        {
          int32_t tmp = y1;
          y1 = y2;
          y2 = tmp;
        }
        y = y1;
        h = y2 + 1 - y;
        _wave_y[si] = y;
        _wave_h[si] = h;
        if (h > 0)
        { /// draw new wave.
          for (size_t xi = x0; xi < x0 + w; ++xi)
          {
            int32_t py = _prev_y[xi / bw];
            int32_t yy = y;
            gfx->setAddrWindow(xi, yy, 1, h);
            do
            {
              gfx->writeColor((yy < py) ? 0xFFCC33u : 0xFFFFFFu, 1);
            } while (++yy < y + h);
          }
        }
        if ((si & 31) == 31) { gfx->display(); taskYIELD(); }
      }
    }
    gfx->display();
    gfx->endWrite();
  }
};
