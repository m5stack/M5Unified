// External displays can be enabled if necessary
// #include <M5ModuleDisplay.h>
// #include <M5AtomDisplay.h>
// #include <M5UnitGLASS2.h>
// #include <M5UnitOLED.h>
// #include <M5UnitLCD.h>

#include <M5Unified.h>

#include <set>
#include <map>

static void* memory_alloc(size_t size)
{
  return heap_caps_malloc(size, MALLOC_CAP_8BIT);
}

static void memory_free(void* ptr)
{
  heap_caps_free(ptr);
}

struct wav_data_t {
  int16_t* wav = nullptr;
  size_t length = 0;
  size_t latest_index = 0;

  size_t searchEdge(size_t offset, size_t search_length) const {
    int mem_position = latest_index + offset;
    if (mem_position >= length) { mem_position -= length; }
    int mem_difference = 0;
    int position = - 1;
    uint32_t counter[2] = { 0,0 };
    bool prev_sign = false;

    for (size_t i = 0; i < search_length; ++i) {
      size_t idx = latest_index + i;
      if (idx >= length) { idx -= length; }
      int value = wav[idx];
      bool sign = value < 0;
      if (prev_sign != sign) {
        prev_sign = sign;
        if (sign) {  // When changing from positive to negative
          if (position >= 0) {
            int diff = counter[0] + counter[1];
            if (mem_difference < diff) {
              mem_difference = diff;
              mem_position = position;
            }
          }
        }
        counter[sign] = 0;
        if (i >= offset) {
          int pidx = (idx ? idx : length) - 1;
          int cv = abs(wav[idx]);
          int pv = abs(wav[pidx]);
          position = (cv < pv) ? idx : pidx;
        }
      }
      uint32_t v = value * value;
      counter[sign] += v >> 7;
    }
    mem_position -= offset;
    if (mem_position < 0) { mem_position += length; }
    return mem_position;
  }
};

struct fft_data_t {
  float *fdata = nullptr;
  size_t length = 0;
  size_t sample_rate;

  wav_data_t *wav_data = nullptr;
  uint8_t fft_size_bits = 0;

  float getDataByPixel(uint16_t x, uint16_t width) const {
    if (length <= width) {
      int index = x * length / width;
      if (index >= length) { index = length - 1; }
      return fdata[index];
    }
    int index0 = x * length / width;
    int index1 = (x + 1) * length / width;
    if (index0 >= length) { index0 = length - 1; }
    if (index1 >= length) { index1 = length - 1; }
    float value = 0;
    for (int i = index0; i < index1; ++i) {
      if (value < fdata[i]) {
        value = fdata[i];
      }
    }
    return value;
  }
};

class fft_function_t {
public:
  bool setup(uint8_t max_fft_size_bits)
  {
    close();
    int FFT_SIZE = 1 << max_fft_size_bits;
    int need_size = 1+(FFT_SIZE * 3 / 4) * sizeof(float) + (FFT_SIZE * sizeof(uint16_t));
    _work_area = memory_alloc(need_size);
    if (_work_area == nullptr) {
      return false;
    }
    _max_fft_size_bits = max_fft_size_bits;
    _initialized_fft_size_bits = 0;

    fi = (float*)memory_alloc(sizeof(float) * FFT_SIZE + 1);
    fr = (float*)memory_alloc(sizeof(float) * FFT_SIZE + 1);

    return true;
  }

  void close(void)
  {
    if (_work_area) {
      memory_free(_work_area);
      _work_area = nullptr;
    }
    _max_fft_size_bits = 0;
    _initialized_fft_size_bits = 0;
  }

  __attribute((optimize("-O3")))
  bool update(fft_data_t* fft_data)
  {
    if (_work_area == nullptr || fft_data == nullptr || fft_data->fdata == nullptr || fft_data->fft_size_bits == 0) {
      return false;
    }
    if (_initialized_fft_size_bits != fft_data->fft_size_bits) {
      if (!_init(fft_data->fft_size_bits)) {
        return false;
      }
    }
    int FFT_SIZE = 1 << fft_data->fft_size_bits;

    uint16_t *br = (uint16_t*)_work_area;
    {
      auto src = fft_data->wav_data->wav;
      for (int i = 0; i < FFT_SIZE; ++i)
      {
        float lv = src[i];
        fr[br[i]] = lv;
      }
      memset(fi, 0, sizeof(float) * FFT_SIZE);
    }
    int s4 = FFT_SIZE / 4;

    float *wi = (float*) (&br[FFT_SIZE]);
    size_t s = 1;
    size_t i = 0;
    size_t je = FFT_SIZE;
    do
    {
      size_t ke = s;
      s <<= 1;
      je >>= 1;
      size_t j = 0;
      do
      {
        size_t k = 0;
        size_t m = ke * ((j << 1) + 1);
        size_t l = s * j;
        auto frm_p = &fr[m];
        auto fim_p = &fi[m];
        auto frl_p = &fr[l];
        auto fil_p = &fi[l];
        auto wi_p = &wi[0];
        auto wr_p = &wi[s4];
        do
        {
          // size_t p = je * k;
          float wi = *wi_p;
          float wr = *wr_p;
          float frm = *frm_p;
          float fim = *fim_p;
          float Wxmr = frm * wr + fim * wi;
          float Wxmi = fim * wr - frm * wi;
          float frl = *frl_p;
          float fil = *fil_p;
          *frm_p++ = frl - Wxmr;
          *frl_p++ = frl + Wxmr;
          *fim_p++ = fil - Wxmi;
          *fil_p++ = fil + Wxmi;
          wi_p += je;
          wr_p += je;
        } while ( ++k < ke);
      } while ( ++j < je );
    } while ( ++i < _initialized_fft_size_bits );


    int loop_end = fft_data->length;
    if (loop_end > FFT_SIZE) {
        loop_end = FFT_SIZE;
    }

    float max_value = 0.0f;
    for (uint32_t i = 0; i < loop_end; ++i) {
      float vr = fr[i];
      float vi = fi[i];
      float v = sqrtf(vr * vr + vi * vi);
      fft_data->fdata[i] = v;
      if (max_value < v) { max_value = v; }
    }

// peak level adjust 
    float k = (65536.0f) / max_value;
    if (k > 0.03125f) { k = 0.03125f; } else
    {
      for (uint32_t i = 0; i < loop_end; ++i) {
        float tmp = fft_data->fdata[i];
        tmp *= k;
        fft_data->fdata[i] = tmp;
      }
    }
//*/
    return true;
  }


private:
  bool _init(uint8_t size_bits)
  {
    if (_max_fft_size_bits < size_bits) { return false; }
    _initialized_fft_size_bits = size_bits;

    uint16_t *br = (uint16_t*)_work_area;

    int FFT_SIZE = 1 << size_bits;
    float *wi = (float*) (&br[FFT_SIZE]);

    size_t je = 1;
    br[0] = 0;
    br[1] = FFT_SIZE >> 1;
    for ( size_t i = 0 ; i < size_bits - 1 ; ++i )
    {
        br[ je << 1 ] = br[ je ] >> 1;
        je = je << 1;
        for ( size_t j = 1 ; j < je ; ++j )
        {
            br[je + j] = br[je] + br[j];
        }
    }

    float omega = 2.0f * M_PI / FFT_SIZE;
    int s2 = FFT_SIZE >> 1;
    int s4 = FFT_SIZE >> 2;
    wi[0] = 0;
    wi[s2] = 0;
    wi[s4] = 1;
    for ( int i = 1 ; i < s4 ; ++i)
    {
        float f = cosf(omega * i);
        wi[s4 + i] = f;
        wi[s4 - i] = f;
        wi[s4+s2 - i] = -f;
    }

    return true;
  }


  float* fi;
  float* fr;
  void* _work_area = nullptr;
  uint8_t _max_fft_size_bits = 0;
  uint8_t _initialized_fft_size_bits;
};


struct rect_t
{
  int16_t x;
  int16_t y;
  int16_t w;
  int16_t h;
};


class wav_drawer_t
{
  LGFX_Device* _gfx = nullptr;
  int16_t* prev_y = nullptr;
  int16_t* prev_h = nullptr;
  rect_t draw_rect = {0, 0, 0, 0};
  uint32_t bg_color = 0x000000u;
  uint32_t fg_color = 0xFFFFFFu;
  uint32_t line_color = 0x303030u;

public:
  bool setup(LGFX_Device* gfx, const rect_t& rect)
  {
    if (gfx == nullptr) { return false; }
    _gfx = gfx;
    draw_rect = rect;
    gfx->fillRect(rect.x, rect.y, rect.w, rect.h, bg_color);
    gfx->drawFastVLine(rect.x + (rect.w >> 1), rect.y, rect.h, line_color);
    int width = rect.w;
    prev_y = (int16_t*)memory_alloc(width * sizeof(int16_t));
    prev_h = (int16_t*)memory_alloc(width * sizeof(int16_t));
    memset(prev_y, 0, width * sizeof(int16_t));
    memset(prev_h, 0, width * sizeof(int16_t));

    return true;
  }

  bool update(const wav_data_t& wav_data)
  {
    auto gfx = _gfx;

    int32_t width = draw_rect.w;
    int32_t height = draw_rect.h;

    int wav_count = wav_data.length;
    int wav_index = wav_data.searchEdge(draw_rect.w >> 1, wav_count / 3);
    auto wav = wav_data.wav;

    int32_t max_value = 1;
    auto wi = wav_index;
    for (int i = 0; i < width; ++i)
    {
      int32_t tmp = abs(wav[wi]);
      if (max_value < tmp) { max_value = tmp; }
      if (++wi >= wav_count) { wi = 0; }
    }
    int new_k = (draw_rect.h << 15) / max_value;
    if (new_k > 65536) 
    { new_k = 65536; }
    static int k;
    if (k > new_k) { k = new_k; }
    else {
      k = (k * 127 + new_k) >> 7;
    }
    int32_t value1 = (32768-wav[wav_index] * k) >> 16;
    int32_t value2 = value1;
    int32_t base_y = draw_rect.y + (height >> 1);

    for (int i = 0; i < width; ++i)
    {
      if (++wav_index >= wav_count) { wav_index = 0; }
      int32_t x = i + draw_rect.x;
      int32_t y = prev_y[i];
      int32_t h = prev_h[i];

      gfx->setColor(i == (width >> 1) ? line_color : bg_color);
      gfx->drawFastVLine(x, base_y + y, h);

      int32_t value0 = value1;
      value1 = value2;
      value2 = (32768-wav[wav_index] * k) >> 16;
      int32_t value_01 = (value0 + value1) >> 1;
      int32_t value_12 = (value1 + value2) >> 1;

      int32_t y_min = (value_01 < value_12) ? value_01 : value_12;
      if (y_min > value1) { y_min = value1; }

      int32_t y_max = (value_01 > value_12) ? value_01 : value_12;
      if (y_max < value1) { y_max = value1; }

      y = y_min;
      h = y_max + 1 - y;
      prev_y[i] = y;
      prev_h[i] = h;
      gfx->drawPixel(x, base_y, line_color);
      /// draw new wave.
      gfx->drawFastVLine(x, base_y + y, h, fg_color);
    }

    return true;
  }
};

class fft_drawer_t
{
  LGFX_Device* _gfx = nullptr;

  uint16_t* prev_y = nullptr;
  uint16_t* prev_h = nullptr;
  rect_t draw_rect = {0, 0, 0, 0};
  uint32_t bg_color = 0x000066u;
  uint32_t fg_color = 0x00FF00u;

public:
  bool setup(LGFX_Device* gfx, const rect_t& rect)
  {
    if (gfx == nullptr) { return false; }
    _gfx = gfx;
    draw_rect = rect;
    gfx->fillRect(rect.x, rect.y, rect.w, rect.h, bg_color);
    int width = rect.w;

    prev_y = (uint16_t*)memory_alloc(width * sizeof(int16_t));
    prev_h = (uint16_t*)memory_alloc(width * sizeof(int16_t));
    memset(prev_y, 0, width * sizeof(int16_t));
    memset(prev_h, 0, width * sizeof(int16_t));
    return true;
  }

  bool update(const fft_data_t& fft_data)
  {
    auto gfx = _gfx;

    int32_t width = draw_rect.w;
    int32_t height = draw_rect.h - 1;

    int32_t value1 = height - (((int32_t)(fft_data.getDataByPixel(0, width) * height)) >> 16);
    if (value1 < 0) { value1 = 0; }
    int32_t value2 = value1;

    for (int i = 0; i < width; ++i)
    {
      int32_t x = i + draw_rect.x;
      int32_t y = prev_y[i];
      int32_t h = prev_h[i];

      gfx->drawFastVLine(x, draw_rect.y + y, h, bg_color);

      int32_t value0 = value1;
      value1 = value2;
      value2 = height - (((int32_t)(fft_data.getDataByPixel(i+1, width) * height)) >> 16);
      if (value2 < 0) { value2 = 0; }

      int32_t value_01 = (value0 + value1) >> 1;
      int32_t value_12 = (value1 + value2) >> 1;

      int32_t y_min = (value_01 < value_12) ? value_01 : value_12;
      if (y_min > value1) { y_min = value1; }

      int32_t y_max = (value_01 > value_12) ? value_01 : value_12;
      if (y_max < value1) { y_max = value1; }

      y = y_min;
      h = y_max + 1 - y;
      prev_y[i] = y;
      prev_h[i] = h;

      gfx->drawFastVLine(x, draw_rect.y + y, h, fg_color);
    }

    return true;
  }
};

class fft_peak_t
{
  LGFX_Device* _gfx = nullptr;

  rect_t draw_rect = {0, 0, 0, 0};
  uint32_t bg_color = 0x000000u;
  uint32_t fg_color = 0x00FFFFu;
  std::set<uint16_t> peak_index_set;
  uint16_t prev_peak_index = UINT16_MAX;
  char text_buf[10] = {0,};
  char prev_text[10] = {0,};
  uint8_t step = 0;
public:
  bool setup(LGFX_Device* gfx, const rect_t& rect)
  {
    if (gfx == nullptr) { return false; }
    _gfx = gfx;
    draw_rect = rect;
    gfx->fillRect(rect.x, rect.y, rect.w, rect.h, bg_color);

    return true;
  }

  bool update(const fft_data_t& fft_data)
  {
    auto gfx = _gfx;

    auto fdata = fft_data.fdata;

    int32_t loop_end = fft_data.length;

    auto boarder_value = (uint16_t*)memory_alloc(sizeof(uint16_t) * loop_end);
    memset(boarder_value, 0, sizeof(uint16_t) * loop_end);
    {
      float thresh = 0;
      for (int i = 0; i < loop_end; ++i)
      {
        float value = fdata[i];
        if (thresh < value) {
          thresh = value;
        }
        boarder_value[i] = thresh;
        thresh = thresh * 0.9f;
      }
      for (int i = loop_end - 1; i >= 0; --i)
      {
        int value = fdata[i];
        if (thresh < value) {
          thresh = value;
        }
        if (boarder_value[i] < thresh) {
          boarder_value[i] = thresh;
        }
        thresh = thresh * 0.9f;
      }
    }
    static constexpr const size_t peak_index_set_size = 8;
    std::multimap<float, uint16_t> peak_map;
    for (int i = 0; i < loop_end; ++i)
    {
      float value = fdata[i];
      if (value >= boarder_value[i]) {
        peak_map.insert(std::make_pair(value, i));
        if (peak_map.size() > peak_index_set_size) {
          peak_map.erase(peak_map.begin());
        }
      }
    }
    memory_free(boarder_value);
    if (peak_map.empty()) {
      return false;
    }

    for (int i = 0; i < 8; ++i)
    {
      auto c = text_buf[step];
      if (prev_text[step] != c) {
        prev_text[step] = c;
        gfx->drawChar(c, draw_rect.x + (step * draw_rect.w >> 3), draw_rect.y);
        break;
      }
      if (++step >= 8)
      {
        step = 0;
        uint16_t peak_index = peak_map.rbegin()->second;
        if (prev_peak_index != peak_index) {
          prev_peak_index = peak_index;
          gfx->setTextDatum(m5gfx::datum_t::top_right);
          float freq = (float)(fft_data.sample_rate * peak_index) / (float)(1<<fft_data.fft_size_bits);
          snprintf(text_buf, sizeof(text_buf), "%8.1f", freq);
        }
      }
    }

    int width = draw_rect.w;
    int icon_size = peak_index_set_size;
    for (auto peak_index : peak_index_set)
    {
      int x = draw_rect.x + (peak_index * width / fft_data.length);
      int y = draw_rect.y + draw_rect.h - icon_size - 1;
      gfx->fillRect(x - icon_size, y, icon_size * 2 + 1, icon_size+1, bg_color);
    }
    peak_index_set.clear();

    for (auto it = peak_map.rbegin(); it != peak_map.rend(); ++it)
    {
      int peak_index = it->second;
      int x = draw_rect.x + (peak_index * width / fft_data.length);
      int y = draw_rect.y + draw_rect.h - 1;
      gfx->drawTriangle(x, y, x - icon_size, y - icon_size, x + icon_size, y - icon_size, fg_color);
      peak_index_set.insert(peak_index);
      icon_size -= 2;
      if (icon_size <= 0) {
        break;
      }
    }

    return true;
  }
};

class fft_history_t
{
  LGFX_Device* _gfx = nullptr;

  uint8_t* color_map = nullptr;
  rect_t draw_rect = {0, 0, 0, 0};
  uint32_t bg_color = 0x000033u;
  uint32_t fg_color = 0xFFFF00u;
  int step = 0;
public:
  bool setup(LGFX_Device* gfx, const rect_t& rect)
  {
    if (gfx == nullptr) { return false; }
    _gfx = gfx;
    draw_rect = rect;
    int width = rect.w;
    int height = rect.h;

    color_map = (uint8_t*)memory_alloc(width * height * sizeof(uint8_t));
    memset(color_map, 0, width * height * sizeof(uint8_t));
    return true;
  }

  bool update(const fft_data_t& fft_data)
  {
    int32_t width = draw_rect.w;
    int32_t height = draw_rect.h;

    if (--step < 0) {
      step = 7;
      memmove(&color_map[width], color_map, width * (height - 1));
      memset(color_map, 0, width);
    }
    for (int i = 0; i < width; ++i)
    {
      int v = fft_data.getDataByPixel(i, width) / 256.0f;
      if (v > 255) { v = 255; }
      color_map[i] = color_map[i] > v ? color_map[i] : v;
    }
    int y0 = ( step      * draw_rect.h) >> 3;
    int y1 = ((step + 1) * draw_rect.h) >> 3;
    _gfx->pushGrayscaleImage(draw_rect.x, draw_rect.y + y0, draw_rect.w, y1 - y0, &color_map[y0*width], m5gfx::color_depth_t::grayscale_8bit, fg_color, bg_color);

    return true;
  }
};

// The higher the sample rate, the higher the frequency results obtained by FFT.
// If limited to the audible range, 24kHz to 48kHz is sufficient.
static constexpr const size_t SAMPLE_RATE = 24000;
// static constexpr const size_t SAMPLE_RATE = 96000;

// The larger the FFT_BITS, the higher the accuracy of the FFT, but the processing load also increases
static constexpr const size_t FFT_BITS = 10;

// WAVE_BLOCK_SIZE is the size of one processing when capturing data from I2S.
// If it is too large, the loop cycle will be slow and the frequency of drawing updates will decrease.
// If it is too small, recoding interruptions will occur.
// For example, if the sample rate is 96kHz and the block size is 384, data will be captured every 4ms.
// Therefore, the drawing process, FFT process, and other loop iterations must be completed within 4ms.
static constexpr const size_t WAVE_BLOCK_SIZE = 256;

static constexpr const size_t FFT_SIZE = 1u << FFT_BITS;
static constexpr const size_t WAVE_BLOCK_COUNT = 3 + FFT_SIZE / WAVE_BLOCK_SIZE;
static constexpr const size_t WAVE_TOTAL_SIZE = WAVE_BLOCK_SIZE * WAVE_BLOCK_COUNT;
static fft_function_t fft_function;
static fft_data_t fft_data;
static wav_data_t wav_data;
static wav_drawer_t wav_drawer;
static fft_drawer_t fft_drawer;
static fft_peak_t fft_peak;
static fft_history_t fft_history;

void setup(void)
{
  auto cfg = M5.config();

#if defined ( __M5GFX_M5MODULEDISPLAY__ )
  cfg.module_display.logical_width = 320;
  cfg.module_display.logical_height = 180;
#endif
#if defined ( __M5GFX_M5ATOMDISPLAY__ )
  cfg.atom_display.logical_width = 320;
  cfg.atom_display.logical_height = 180;
#endif

  // use for ATOMIC ECHO BASE
  cfg.external_speaker.atomic_echo = true;

  M5.begin(cfg);
  M5.setPrimaryDisplayType({m5gfx::board_M5ModuleDisplay, m5gfx::board_M5AtomDisplay});
  M5.Speaker.tone(440, 100);
  M5.delay(100);
  {
    auto cfg = M5.Mic.config();
    cfg.dma_buf_count = 3;
    cfg.dma_buf_len = WAVE_BLOCK_SIZE;
    cfg.over_sampling = 1;
    cfg.noise_filter_level = 0;
    cfg.sample_rate = SAMPLE_RATE;
    cfg.magnification = cfg.use_adc ? 16 : 1;

/*
    { // use for Unit PDM ( Port A )
      cfg.pin_data_in = M5.getPin(m5::pin_name_t::port_a_pin2);
      cfg.pin_ws = M5.getPin(m5::pin_name_t::port_a_pin1);
      cfg.pin_bck = -1;
      cfg.pin_mck = -1;
      cfg.use_adc = false;
      cfg.stereo = false;
      cfg.i2s_port = i2s_port_t::I2S_NUM_0;
    }
//*/
    M5.Mic.config(cfg);
  }

  if (!M5.Mic.isEnabled()) {
    M5.Display.printf("microphone is not available.");
    M5_LOGE("microphone is not available.");
    for (;;) {
      M5.delay(256);
    }
  }
  M5.Speaker.end();
  M5.Mic.begin();

  M5.Display.startWrite();

  int16_t w = M5.Display.width();
  int16_t h = M5.Display.height() >> 2;
  int16_t y = 0;

  rect_t rect_fft_peak = {0, y, w, h};
  y += h;
  rect_t rect_fft_drawer = {0, y, w, h};
  y += h;
  rect_t rect_fft_history = {0, y, w, h};
  y += h;
  rect_t rect_wav_drawer = {0, y, w, h};

  fft_function.setup(FFT_BITS);
  fft_peak.setup(&M5.Display, rect_fft_peak);
  fft_drawer.setup(&M5.Display, rect_fft_drawer);
  fft_history.setup(&M5.Display, rect_fft_history);
  wav_drawer.setup(&M5.Display, rect_wav_drawer);

  M5.Display.setTextSize(w / 64.0f, h / 16.0f);
  M5.Display.setFont(&fonts::AsciiFont8x16);
  M5.Display.setEpdMode(epd_mode_t::epd_fastest);

  fft_data.fft_size_bits = FFT_BITS;
  fft_data.sample_rate = SAMPLE_RATE;
  fft_data.wav_data = &wav_data;
  fft_data.length = (1 << (fft_data.fft_size_bits - 1)) + 1;
  fft_data.fdata = (typeof(fft_data.fdata))memory_alloc(fft_data.length * sizeof(fft_data.fdata[0]));

  wav_data.length = WAVE_TOTAL_SIZE;
  wav_data.wav = (typeof(wav_data.wav))memory_alloc(WAVE_TOTAL_SIZE * sizeof(wav_data.wav[0]));
  memset(wav_data.wav, 0 , WAVE_TOTAL_SIZE * sizeof(int16_t));
}

void loop(void)
{
  static int step = -1;

  if (M5.Mic.isEnabled())
  {
    int wav_idx = wav_data.latest_index;
    while (M5.Mic.isRecording() < 2) {
      M5.Mic.record(&(wav_data.wav[wav_idx]), WAVE_BLOCK_SIZE, SAMPLE_RATE, false);
      wav_idx += WAVE_BLOCK_SIZE;
      if (wav_idx >= WAVE_TOTAL_SIZE) { wav_idx = 0; }
      wav_data.latest_index = wav_idx;
    };

    {
      switch (++step) {
      default:
        step = 0;
        M5.Display.display();
        fft_function.update(&fft_data);
        break;
      case 1:
        wav_drawer.update(wav_data);
        break;
      case 2:
        fft_drawer.update(fft_data);
        break;
      case 3:
        fft_history.update(fft_data);
        break;
      case 4:
        fft_peak.update(fft_data);
        break;
      }
    }
  }
}

// for ESP-IDF
extern "C" {
  __attribute__((weak))
  void app_main()
  {
    setup();
    for (;;) {
      loop();
    }
  }
}
