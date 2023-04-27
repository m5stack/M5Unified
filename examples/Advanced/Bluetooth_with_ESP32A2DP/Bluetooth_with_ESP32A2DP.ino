
#include <M5UnitLCD.h>
#include <M5UnitOLED.h>
#include <M5Unified.h>

/// need ESP32-A2DP library. ( URL : https://github.com/pschatzmann/ESP32-A2DP/ )
#include <BluetoothA2DPSink.h>

/// set M5Speaker virtual channel (0-7)
static constexpr uint8_t m5spk_virtual_channel = 0;

/// set ESP32-A2DP device name
static constexpr char bt_device_name[] = "ESP32";


class BluetoothA2DPSink_M5Speaker : public BluetoothA2DPSink
{
public:
  BluetoothA2DPSink_M5Speaker(m5::Speaker_Class* m5sound, uint8_t virtual_channel = 0)
  : BluetoothA2DPSink()
  {
    is_i2s_output = false; // I2S control by BluetoothA2DPSink is not required.
  }

  // get rawdata buffer for FFT.
  const int16_t* getBuffer(void) const { return _tri_buf[_export_index]; }

  const char* getMetaData(size_t id, bool clear_flg = true) { if (clear_flg) { _meta_bits &= ~(1<<id); } return (id < metatext_num) ? _meta_text[id] : nullptr; }

  uint8_t getMetaUpdateInfo(void) const { return _meta_bits; }
  void clearMetaUpdateInfo(void) { _meta_bits = 0; }

  void clear(void)
  {
    for (int i = 0; i < 3; ++i)
    {
      if (_tri_buf[i]) { memset(_tri_buf[i], 0, _tri_buf_size[i]); }
    }
  }

  static constexpr size_t metatext_size = 128;
  static constexpr size_t metatext_num = 3;

protected:
  int16_t* _tri_buf[3] = { nullptr, nullptr, nullptr };
  size_t _tri_buf_size[3] = { 0, 0, 0 };
  size_t _tri_index = 0;
  size_t _export_index = 0;
  char _meta_text[metatext_num][metatext_size];
  uint8_t _meta_bits = 0;
  size_t _sample_rate = 48000;

  void clearMetaData(void)
  {
    for (int i = 0; i < metatext_num; ++i)
    {
      _meta_text[i][0] = 0;
    }
    _meta_bits = (1<<metatext_num)-1;
  }

  void av_hdl_a2d_evt(uint16_t event, void *p_param) override
  {
    esp_a2d_cb_param_t* a2d = (esp_a2d_cb_param_t *)(p_param);

    switch (event) {
    case ESP_A2D_CONNECTION_STATE_EVT:
      if (ESP_A2D_CONNECTION_STATE_CONNECTED == a2d->conn_stat.state)
      { // 接続

      }
      else
      if (ESP_A2D_CONNECTION_STATE_DISCONNECTED == a2d->conn_stat.state)
      { // 切断

      }
      break;

    case ESP_A2D_AUDIO_STATE_EVT:
      if (ESP_A2D_AUDIO_STATE_STARTED == a2d->audio_stat.state)
      { // 再生

      } else
      if ( ESP_A2D_AUDIO_STATE_REMOTE_SUSPEND == a2d->audio_stat.state
        || ESP_A2D_AUDIO_STATE_STOPPED        == a2d->audio_stat.state )
      { // 停止
        clearMetaData();
        clear();
      }
      break;

    case ESP_A2D_AUDIO_CFG_EVT:
      {
        esp_a2d_cb_param_t *a2d = (esp_a2d_cb_param_t *)(p_param);
        size_t tmp = a2d->audio_cfg.mcc.cie.sbc[0];
        size_t rate = 16000;
        if (     tmp & (1 << 6)) { rate = 32000; }
        else if (tmp & (1 << 5)) { rate = 44100; }
        else if (tmp & (1 << 4)) { rate = 48000; }
        _sample_rate = rate;
      }
      break;

    default:
      break;
    }

    BluetoothA2DPSink::av_hdl_a2d_evt(event, p_param);
  }

  void av_hdl_avrc_evt(uint16_t event, void *p_param) override
  {
    esp_avrc_ct_cb_param_t *rc = (esp_avrc_ct_cb_param_t *)(p_param);

    switch (event)
    {
    case ESP_AVRC_CT_METADATA_RSP_EVT:
      for (size_t i = 0; i < metatext_num; ++i)
      {
        if (0 == (rc->meta_rsp.attr_id & (1 << i))) { continue; }
        strncpy(_meta_text[i], (char*)(rc->meta_rsp.attr_text), metatext_size);
        _meta_bits |= rc->meta_rsp.attr_id;
        break;
      }
      break;

    case ESP_AVRC_CT_CONNECTION_STATE_EVT:
      break;

    case ESP_AVRC_CT_CHANGE_NOTIFY_EVT:
      break;

    default:
      break;
    }

    BluetoothA2DPSink::av_hdl_avrc_evt(event, p_param);
  }

  int16_t* get_next_buf(const uint8_t* src_data, uint32_t len)
  {
    size_t tri = _tri_index < 2 ? _tri_index + 1 : 0;
    if (_tri_buf_size[tri] < len)
    {
      _tri_buf_size[tri] = len;
      if (_tri_buf[tri] != nullptr) { heap_caps_free(_tri_buf[tri]); }
      auto tmp = (int16_t*)heap_caps_malloc(len, MALLOC_CAP_8BIT);
      _tri_buf[tri] = tmp;
      if (tmp == nullptr)
      {
        _tri_buf_size[tri] = 0;
        return nullptr;
      }
    }
    memcpy(_tri_buf[tri], src_data, len);
    _tri_index = tri;
    return _tri_buf[tri];
  }

  void audio_data_callback(const uint8_t *data, uint32_t length) override
  {
    // Reduce memory requirements by dividing the received data into the first and second halves.
    length >>= 1;
    M5.Speaker.playRaw(get_next_buf( data        , length), length >> 1, _sample_rate, true, 1, m5spk_virtual_channel);
    M5.Speaker.playRaw(get_next_buf(&data[length], length), length >> 1, _sample_rate, true, 1, m5spk_virtual_channel);
    _export_index = _tri_index;
  }
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

static constexpr size_t WAVE_SIZE = 320;
static BluetoothA2DPSink_M5Speaker a2dp_sink = { &M5.Speaker, m5spk_virtual_channel };
static fft_t fft;
static bool fft_enabled = false;
static bool wave_enabled = false;
static uint16_t prev_y[(FFT_SIZE / 2)+1];
static uint16_t peak_y[(FFT_SIZE / 2)+1];
static int16_t wave_y[WAVE_SIZE];
static int16_t wave_h[WAVE_SIZE];
static int16_t raw_data[WAVE_SIZE * 2];
static int header_height = 0;


uint32_t bgcolor(LGFX_Device* gfx, int y)
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

void gfxSetup(LGFX_Device* gfx)
{
  if (gfx == nullptr) { return; }
  if (gfx->width() < gfx->height())
  {
    gfx->setRotation(gfx->getRotation()^1);
  }
  gfx->setFont(&fonts::lgfxJapanGothic_12);
  gfx->setEpdMode(epd_mode_t::epd_fastest);
  gfx->setCursor(0, 8);
  gfx->print("BT A2DP : ");
  gfx->println(bt_device_name);
  gfx->setTextWrap(false);
  gfx->fillRect(0, 6, gfx->width(), 2, TFT_BLACK);

  header_height = (gfx->height() > 80) ? 45 : 21;
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
    auto bits = a2dp_sink.getMetaUpdateInfo();
    if (bits)
    {
      gfx->startWrite();
      for (int id = 0; id < a2dp_sink.metatext_num; ++id)
      {
        if (0 == (bits & (1<<id))) { continue; }
        size_t y = id * 12;
        if (y+12 >= header_height) { continue; }
        gfx->setCursor(4, 8 + y);
        gfx->fillRect(0, 8 + y, gfx->width(), 12, gfx->getBaseColor());
        gfx->print(a2dp_sink.getMetaData(id));
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

    if (a2dp_sink.getMetaUpdateInfo())
    {
      gfx->fillRect(0, 8, gfx->width(), 12, TFT_BLACK);
      a2dp_sink.clearMetaUpdateInfo();
      title_x = 4;
      title_id = 0;
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
        const char* meta = a2dp_sink.getMetaData(tid, false);
        if (meta[0] != 0)
        {
          gfx->print(meta);
          gfx->print("  /  ");
          tx = gfx->getCursorX();
          if (++tid == a2dp_sink.metatext_num) { tid = 0; }
          if (tx <= 4)
          {
            title_x = tx;
            title_id = tid;
          }
        }
        else
        {
          if ((no_data_bits |= 1 << tid) == ((1 << a2dp_sink.metatext_num) - 1))
          {
            break;
          }
          if (++tid == a2dp_sink.metatext_num) { tid = 0; }
        }
      } while (tx < gfx->width());
      --title_x;
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

  if (fft_enabled && !gfx->displayBusy())
  {
    static int prev_x[2];
    static int peak_x[2];
    static bool prev_conn;
    bool connected = a2dp_sink.is_connected();
    if (prev_conn != connected)
    {
      prev_conn = connected;
      if (!connected)
      {
        a2dp_sink.clear();
      }
    }

    auto buf = a2dp_sink.getBuffer();
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


  { /// custom setting
    auto spk_cfg = M5.Speaker.config();
    /// Increasing the sample_rate will improve the sound quality instead of increasing the CPU load.
    spk_cfg.sample_rate = 96000; // default:64000 (64kHz)  e.g. 48000 , 50000 , 80000 , 96000 , 100000 , 128000 , 144000 , 192000 , 200000
    spk_cfg.task_pinned_core = APP_CPU_NUM;
    // spk_cfg.task_priority = configMAX_PRIORITIES - 2;
    spk_cfg.dma_buf_count = 20;
    // spk_cfg.dma_buf_len = 512;
    M5.Speaker.config(spk_cfg);
  }


  M5.Speaker.begin();

  a2dp_sink.start(bt_device_name, false);

  gfxSetup(&M5.Display);
}

void loop(void)
{
  gfxLoop(&M5.Display);

  {
    static int prev_frame;
    int frame;
    do
    {
      vTaskDelay(1);
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
      a2dp_sink.next();
      break;

    case 2:
      M5.Speaker.tone(800, 100);
      a2dp_sink.previous();
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

#if !defined ( ARDUINO )
extern "C" {
  void loopTask(void*)
  {
    setup();
    for (;;) {
      loop();
    }
    vTaskDelete(NULL);
  }

  void app_main()
  {
    xTaskCreatePinnedToCore(loopTask, "loopTask", 8192, NULL, 1, NULL, 1);
  }
}
#endif
