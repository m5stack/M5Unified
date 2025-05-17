// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "Mic_Class.hpp"

#include "../M5Unified.hpp"

#if __has_include (<esp_idf_version.h>)
 #include <esp_idf_version.h>
 #if ESP_IDF_VERSION_MAJOR >= 4
  #define NON_BREAK ;[[fallthrough]];
 #endif
#endif

#if __has_include (<soc/pcr_struct.h>)
#include <soc/pcr_struct.h>
#endif

#ifndef NON_BREAK
#define NON_BREAK ;
#endif

#if __has_include(<sdkconfig.h>)
#include <sdkconfig.h>
#include <esp_log.h>
#include <math.h>

#if defined ( CONFIG_IDF_TARGET_ESP32C3 ) || defined ( CONFIG_IDF_TARGET_ESP32C6 ) || defined ( CONFIG_IDF_TARGET_ESP32S3 ) || defined ( CONFIG_IDF_TARGET_ESP32P4 )
 #if __has_include(<driver/i2s_std.h>)
  #if __has_include(<hal/i2s_ll.h>)
   #include <hal/i2s_ll.h>
  #endif
 #endif
#endif

#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)  
#if __has_include (<hal/adc_ll.h>)
 #pragma GCC diagnostic push
 #pragma GCC diagnostic ignored "-Wconversion"
 #pragma GCC diagnostic ignored "-Wmissing-field-initializers"

 #include <hal/adc_ll.h>
 #include <driver/rtc_io.h>

 #pragma GCC diagnostic pop
#endif
#endif

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

namespace m5
{
#if defined ( ESP_PLATFORM )
#if defined (ESP_IDF_VERSION_VAL)
 #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0)
  #define COMM_FORMAT_I2S (I2S_COMM_FORMAT_STAND_I2S)
  #define COMM_FORMAT_MSB (I2S_COMM_FORMAT_STAND_MSB)
 #endif
 #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 3, 3)
  #define SAMPLE_RATE_TYPE uint32_t
 #endif
 #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
  #define MIC_CLASS_ADC_WIDTH_BITS ADC_WIDTH_BIT_12
  #define MIC_CLASS_ADC_ATTEN_DB ADC_ATTEN_DB_12
 #endif
#endif

#ifndef COMM_FORMAT_I2S
 #define COMM_FORMAT_I2S (I2S_COMM_FORMAT_I2S)
 #define COMM_FORMAT_MSB (I2S_COMM_FORMAT_I2S_MSB)
#endif

#ifndef SAMPLE_RATE_TYPE
 #define SAMPLE_RATE_TYPE int
#endif

#ifndef MIC_CLASS_ADC_WIDTH_BITS
 #define MIC_CLASS_ADC_WIDTH_BITS ADC_WIDTH_12Bit
 #define MIC_CLASS_ADC_ATTEN_DB ADC_ATTEN_11db
#endif


  uint32_t Mic_Class::_calc_rec_rate(void) const
  {
    int rate = (_cfg.sample_rate * _cfg.over_sampling);
    return rate;
  }

#if __has_include(<driver/i2s_std.h>)

  static i2s_chan_handle_t _i2s_handle[SOC_I2S_NUM] = { nullptr, };

  static esp_err_t _i2s_start(i2s_port_t port) {
    return i2s_channel_enable(_i2s_handle[port]);
  }
  static esp_err_t _i2s_stop(i2s_port_t port)
  {
    return i2s_channel_disable(_i2s_handle[port]);
  }
  static esp_err_t _i2s_read(i2s_port_t port, void* buf, size_t len, size_t* result, TickType_t tick) {
    return i2s_channel_read(_i2s_handle[port], buf, len, result, tick);
  }
  static esp_err_t _i2s_driver_uninstall(i2s_port_t port)
  {
    if (_i2s_handle[port] != nullptr) {
      auto res = i2s_del_channel(_i2s_handle[port]);
      _i2s_handle[port] = nullptr;
      return res;
    }
    return ESP_OK;
  }
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)  

  struct adc_digi_pattern_table_t {
      union {
          struct {
              uint8_t atten:     2;   /*!< ADC sampling voltage attenuation configuration. Modification of attenuation affects the range of measurements.
                                          0: measurement range 0 - 800mV,
                                          1: measurement range 0 - 1100mV,
                                          2: measurement range 0 - 1350mV,
                                          3: measurement range 0 - 2600mV. */
              uint8_t bit_width: 2;   /*!< ADC resolution.
  -                                         0: 9 bit;
  -                                         1: 10 bit;
  -                                         2: 11 bit;
  -                                         3: 12 bit. */
              int8_t channel:   4;   /*!< ADC channel index. */
          };
          uint8_t val;                /*!<Raw data value */
      };
  };

  static esp_err_t _i2s_set_adc(i2s_port_t port, gpio_num_t pin_data_in) {
    if (port == I2S_NUM_0)
    { /// レジスタを操作してADCモードの設定を有効にする ;
      if (((size_t)pin_data_in) > 39) { return ESP_FAIL; }
      static constexpr const uint8_t adc_table[] =
      {
        ADC2_GPIO0_CHANNEL , // GPIO  0
        255            ,
        ADC2_GPIO2_CHANNEL , // GPIO  2
        255            ,
        ADC2_GPIO4_CHANNEL , // GPIO  4
        255, 255, 255, 255, 255, 255, 255,
        ADC2_GPIO12_CHANNEL , // GPIO 12
        ADC2_GPIO13_CHANNEL , // GPIO 13
        ADC2_GPIO14_CHANNEL , // GPIO 14
        ADC2_GPIO15_CHANNEL , // GPIO 15
        255, 255, 255, 255, 255, 255, 255, 255, 255,
        ADC2_GPIO25_CHANNEL , // GPIO 25
        ADC2_GPIO26_CHANNEL , // GPIO 26
        ADC2_GPIO27_CHANNEL , // GPIO 27
        255, 255, 255, 255,
        ADC1_GPIO32_CHANNEL , // GPIO 32
        ADC1_GPIO33_CHANNEL , // GPIO 33
        ADC1_GPIO34_CHANNEL , // GPIO 34
        ADC1_GPIO35_CHANNEL , // GPIO 35
        ADC1_GPIO36_CHANNEL , // GPIO 36
        ADC1_GPIO37_CHANNEL , // GPIO 37
        ADC1_GPIO38_CHANNEL , // GPIO 38
        ADC1_GPIO39_CHANNEL , // GPIO 39
      };
      int adc_ch = adc_table[pin_data_in];
      if (adc_ch == 255) { return ESP_FAIL; }

      adc_unit_t unit = pin_data_in >= 32 ? ADC_UNIT_1 : ADC_UNIT_2;

      adc_oneshot_ll_set_output_bits(unit, ADC_BITWIDTH_12);

      {
        rtc_gpio_init(pin_data_in);
        rtc_gpio_set_direction(pin_data_in, RTC_GPIO_MODE_DISABLED);
        rtc_gpio_pullup_dis(pin_data_in);
        rtc_gpio_pulldown_dis(pin_data_in);

        adc_ll_digi_set_fsm_time(8/*ADC_HAL_FSM_RSTB_WAIT_DEFAULT*/, 
                                16/*ADC_HAL_FSM_START_WAIT_DEFAULT*/,
                                100/*ADC_HAL_FSM_STANDBY_WAIT_DEFAULT*/);
        adc_ll_set_sample_cycle(2/*ADC_HAL_SAMPLE_CYCLE_DEFAULT*/);
        adc_ll_pwdet_set_cct(4/*ADC_HAL_PWDET_CCT_DEFAULT*/);
        adc_ll_digi_set_clk_div(16/*ADC_HAL_DIGI_SAR_CLK_DIV_DEFAULT*/);
        adc_ll_digi_output_invert(unit, ADC_LL_DIGI_DATA_INVERT_DEFAULT(unit));
        adc_ll_digi_set_convert_mode((unit == ADC_UNIT_1) ? ADC_LL_DIGI_CONV_ONLY_ADC1 : ADC_LL_DIGI_CONV_ONLY_ADC2);
        adc_ll_set_controller(unit, ADC_LL_CTRL_DIG);
        adc_ll_digi_clear_pattern_table(unit);
        adc_ll_digi_set_pattern_table_len(unit, 1);
        adc_digi_pattern_table_t pattern;
        pattern.atten = ADC_ATTEN_DB_12;
        pattern.bit_width = 3;
        pattern.channel = adc_ch;
        {
          uint8_t index = 0;
          uint8_t offset = 0;
          auto saradc_tab = (adc_ch == ADC_UNIT_1)
                          ? SYSCON.saradc_sar1_patt_tab
                          : SYSCON.saradc_sar2_patt_tab;
          uint32_t tab = saradc_tab[index];           // Read old register value
          tab &= (~(0xFF000000 >> offset));           // clear old data
          tab |= ((uint32_t)pattern.val << 24) >> offset; // Fill in the new data
          saradc_tab[index] = tab;   // Write back
        }
        SYSCON.saradc_ctrl.data_sar_sel = 0; // ADC_DIGI_FORMAT_12BIT;
        adc_ll_digi_convert_limit_enable(ADC_LL_DEFAULT_CONV_LIMIT_EN);
        adc_ll_digi_set_convert_limit_num(ADC_LL_DEFAULT_CONV_LIMIT_NUM);
        adc_ll_digi_set_data_source(1); //ADC_I2S_DATA_SRC_ADC;
      }
      I2S0.conf2.lcd_en = true;
      I2S0.conf.rx_right_first = 0;
      I2S0.conf.rx_msb_shift = 0;
      I2S0.conf.rx_mono = 0;
      I2S0.conf.rx_short_sync = 0;
      I2S0.fifo_conf.rx_fifo_mod = true;
      I2S0.conf_chan.rx_chan_mod = true;

      return ESP_OK;
    }
    return ESP_FAIL;
  }
#endif
#else
  static esp_err_t _i2s_start(i2s_port_t port)
  {
    return i2s_start(port);
  }
  static esp_err_t _i2s_stop(i2s_port_t port)
  {
    return i2s_stop(port);
  }
  static esp_err_t _i2s_read(i2s_port_t port, void* buf, size_t len, size_t* result, TickType_t tick)
  {
    return i2s_read(port, buf, len, result, tick);
  }
  static esp_err_t _i2s_driver_uninstall(i2s_port_t port)
  {
    return i2s_driver_uninstall(port);
  }
#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)  
  static esp_err_t _i2s_set_adc(i2s_port_t port, gpio_num_t pin_data_in) {
    if (port == I2S_NUM_0)
    { /// レジスタを操作してADCモードの設定を有効にする ;
      if (((size_t)pin_data_in) > 39) { return ESP_FAIL; }
      static constexpr const uint8_t adc_table[] =
      {
        ADC2_CHANNEL_1 , // GPIO  0
        255            ,
        ADC2_CHANNEL_2 , // GPIO  2
        255            ,
        ADC2_CHANNEL_0 , // GPIO  4
        255, 255, 255, 255, 255, 255, 255,
        ADC2_CHANNEL_5 , // GPIO 12
        ADC2_CHANNEL_4 , // GPIO 13
        ADC2_CHANNEL_6 , // GPIO 14
        ADC2_CHANNEL_3 , // GPIO 15
        255, 255, 255, 255, 255, 255, 255, 255, 255,
        ADC2_CHANNEL_8 , // GPIO 25
        ADC2_CHANNEL_9 , // GPIO 26
        ADC2_CHANNEL_7 , // GPIO 27
        255, 255, 255, 255,
        ADC1_CHANNEL_4 , // GPIO 32
        ADC1_CHANNEL_5 , // GPIO 33
        ADC1_CHANNEL_6 , // GPIO 34
        ADC1_CHANNEL_7 , // GPIO 35
        ADC1_CHANNEL_0 , // GPIO 36
        ADC1_CHANNEL_1 , // GPIO 37
        ADC1_CHANNEL_2 , // GPIO 38
        ADC1_CHANNEL_3 , // GPIO 39
      };
      int adc_ch = adc_table[pin_data_in];
      if (adc_ch == 255) { return ESP_FAIL; }

      adc_unit_t unit = pin_data_in >= 32 ? ADC_UNIT_1 : ADC_UNIT_2;
      adc_set_data_width(unit, MIC_CLASS_ADC_WIDTH_BITS);
      esp_err_t err = i2s_set_adc_mode(unit, (adc1_channel_t)adc_ch);
      if (unit == ADC_UNIT_1)
      {
        adc1_config_channel_atten((adc1_channel_t)adc_ch, MIC_CLASS_ADC_ATTEN_DB);
      }
      else
      {
        adc2_config_channel_atten((adc2_channel_t)adc_ch, MIC_CLASS_ADC_ATTEN_DB);
      }
      I2S0.conf2.lcd_en = true;
      I2S0.conf.rx_right_first = 0;
      I2S0.conf.rx_msb_shift = 0;
      I2S0.conf.rx_mono = 0;
      I2S0.conf.rx_short_sync = 0;
      I2S0.fifo_conf.rx_fifo_mod = true;
      I2S0.conf_chan.rx_chan_mod = true;
      return err;
    }
    return ESP_FAIL;
  }
#endif

#endif

  esp_err_t Mic_Class::_setup_i2s(void)
  {
    if (_cfg.pin_data_in  < 0) { return ESP_FAIL; }
#if __has_include(<driver/i2s_std.h>)

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(_cfg.i2s_port, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = _cfg.dma_buf_count;
    chan_cfg.dma_frame_num = _cfg.dma_buf_len;
    _i2s_driver_uninstall(_cfg.i2s_port);
    esp_err_t err = i2s_new_channel(&chan_cfg, nullptr, &_i2s_handle[_cfg.i2s_port]);
    if (err != ESP_OK) { return err; }

#if SOC_I2S_SUPPORTS_PDM_RX
if (_cfg.pin_bck < 0 || _cfg.pin_ws < 0) {
    i2s_pdm_rx_config_t i2s_config;
    memset(&i2s_config, 0, sizeof(i2s_pdm_rx_config_t));
#if defined ( CONFIG_IDF_TARGET_ESP32P4 )
    i2s_config.clk_cfg.clk_src = i2s_clock_src_t::I2S_CLK_SRC_DEFAULT;
#else
    i2s_config.clk_cfg.clk_src = i2s_clock_src_t::I2S_CLK_SRC_PLL_160M;
#endif
    i2s_config.clk_cfg.sample_rate_hz = 48000; // dummy setting
    i2s_config.clk_cfg.mclk_multiple = i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_128; // dummy setting
    i2s_config.slot_cfg.data_bit_width = i2s_data_bit_width_t::I2S_DATA_BIT_WIDTH_16BIT;
    i2s_config.slot_cfg.slot_bit_width = i2s_slot_bit_width_t::I2S_SLOT_BIT_WIDTH_16BIT;
    i2s_config.slot_cfg.slot_mode = (_cfg.stereo) ? i2s_slot_mode_t::I2S_SLOT_MODE_STEREO :  i2s_slot_mode_t::I2S_SLOT_MODE_MONO;
    i2s_config.slot_cfg.slot_mask = (_cfg.stereo) ? i2s_pdm_slot_mask_t::I2S_PDM_SLOT_BOTH : (_cfg.left_channel ? i2s_pdm_slot_mask_t::I2S_PDM_SLOT_LEFT : i2s_pdm_slot_mask_t::I2S_PDM_SLOT_RIGHT);
    i2s_config.gpio_cfg.clk = (gpio_num_t)_cfg.pin_ws; 
    i2s_config.gpio_cfg.din = (gpio_num_t)_cfg.pin_data_in;
    err = i2s_channel_init_pdm_rx_mode(_i2s_handle[_cfg.i2s_port], &i2s_config);
} else
#endif
{
    i2s_std_config_t i2s_config;
    memset(&i2s_config, 0, sizeof(i2s_std_config_t));
    #if defined ( CONFIG_IDF_TARGET_ESP32P4 )
    i2s_config.clk_cfg.clk_src = i2s_clock_src_t::I2S_CLK_SRC_DEFAULT;
#else
    i2s_config.clk_cfg.clk_src = i2s_clock_src_t::I2S_CLK_SRC_PLL_160M;
#endif
    i2s_config.clk_cfg.sample_rate_hz = 48000; // dummy setting
    i2s_config.clk_cfg.mclk_multiple = i2s_mclk_multiple_t::I2S_MCLK_MULTIPLE_128; // dummy setting
    i2s_config.slot_cfg.data_bit_width = i2s_data_bit_width_t::I2S_DATA_BIT_WIDTH_16BIT;
    i2s_config.slot_cfg.slot_bit_width = i2s_slot_bit_width_t::I2S_SLOT_BIT_WIDTH_16BIT;
    i2s_config.slot_cfg.slot_mode = (_cfg.stereo) ? i2s_slot_mode_t::I2S_SLOT_MODE_STEREO :  i2s_slot_mode_t::I2S_SLOT_MODE_MONO;
    i2s_config.slot_cfg.slot_mask = (_cfg.stereo) ? i2s_std_slot_mask_t::I2S_STD_SLOT_BOTH : (_cfg.left_channel ? i2s_std_slot_mask_t::I2S_STD_SLOT_LEFT : i2s_std_slot_mask_t::I2S_STD_SLOT_RIGHT);
    i2s_config.slot_cfg.ws_width = 16;
    i2s_config.slot_cfg.bit_shift = true;
#if SOC_I2S_HW_VERSION_1    // For esp32/esp32-s2
    i2s_config.slot_cfg.msb_right = false;
#else
    i2s_config.slot_cfg.left_align = true;
    i2s_config.slot_cfg.big_endian = false;
    i2s_config.slot_cfg.bit_order_lsb = false;
#endif
    i2s_config.gpio_cfg.bclk = (gpio_num_t)_cfg.pin_bck;
    i2s_config.gpio_cfg.ws   = (gpio_num_t)_cfg.pin_ws;
    i2s_config.gpio_cfg.dout = (gpio_num_t)I2S_PIN_NO_CHANGE;
    i2s_config.gpio_cfg.mclk = (gpio_num_t)_cfg.pin_mck;
    i2s_config.gpio_cfg.din  = (gpio_num_t)_cfg.pin_data_in;
    err = i2s_channel_init_std_mode(_i2s_handle[_cfg.i2s_port], &i2s_config);
}

#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
    if (_cfg.use_adc)
    {
      err = _i2s_set_adc(_cfg.i2s_port, (gpio_num_t)_cfg.pin_data_in);
    }
#endif

    return err;

#else
    i2s_config_t i2s_config;
    memset(&i2s_config, 0, sizeof(i2s_config_t));
    i2s_config.mode                 = (i2s_mode_t)( I2S_MODE_MASTER | I2S_MODE_RX );
	  i2s_config.sample_rate          = 48000; // dummy setting.
    i2s_config.bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT;
    i2s_config.channel_format       = _cfg.stereo ? I2S_CHANNEL_FMT_RIGHT_LEFT : _cfg.left_channel ? I2S_CHANNEL_FMT_ONLY_LEFT : I2S_CHANNEL_FMT_ONLY_RIGHT;
    i2s_config.communication_format = (i2s_comm_format_t)( COMM_FORMAT_I2S );
    i2s_config.dma_buf_count        = _cfg.dma_buf_count;
    i2s_config.dma_buf_len          = _cfg.dma_buf_len;

    i2s_pin_config_t pin_config;
    memset(&pin_config, ~0u, sizeof(i2s_pin_config_t)); /// all pin set to I2S_PIN_NO_CHANGE
#if defined (ESP_IDF_VERSION_VAL)
 #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 1)
    pin_config.mck_io_num     = _cfg.pin_mck;
 #endif
#endif
    pin_config.bck_io_num     = _cfg.pin_bck;
    pin_config.ws_io_num      = _cfg.pin_ws;
    pin_config.data_in_num    = _cfg.pin_data_in;

    if (_cfg.pin_bck < 0 || _cfg.pin_ws < 0) {
      i2s_config.mode                 = (i2s_mode_t)( I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM );
      i2s_config.communication_format = i2s_comm_format_t::I2S_COMM_FORMAT_STAND_PCM_SHORT;
    }
    esp_err_t err;
    if (ESP_OK != (err = i2s_driver_install(_cfg.i2s_port, &i2s_config, 0, nullptr)))
    {
      _i2s_driver_uninstall(_cfg.i2s_port);
      err = i2s_driver_install(_cfg.i2s_port, &i2s_config, 0, nullptr);
    }
    if (err != ESP_OK) { return err; }

#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
    if (_cfg.use_adc)
    {
      err = _i2s_set_adc(_cfg.i2s_port, (gpio_num_t)_cfg.pin_data_in);
    }
    else
#endif
    {
      err = i2s_set_pin(_cfg.i2s_port, &pin_config);
    }

    return err;
#endif
  }

  // クロックディバイダー計算用関数 (実装は Speaker_Class.cpp内)
  void calcClockDiv(uint32_t* div_a, uint32_t* div_b, uint32_t* div_n, uint32_t baseClock, uint32_t targetFreq);

  void Mic_Class::mic_task(void* args)
  {
    auto self = (Mic_Class*)args;
    int oversampling = self->_cfg.over_sampling;
    if (     oversampling < 1) { oversampling = 1; }
    else if (oversampling > 8) { oversampling = 8; }

    bool use_pdm = (self->_cfg.pin_bck < 0 && !self->_cfg.use_adc);

#if defined ( CONFIG_IDF_TARGET_ESP32C3 ) || defined (CONFIG_IDF_TARGET_ESP32C6) || defined ( CONFIG_IDF_TARGET_ESP32S3 )
    static constexpr uint32_t PLL_D2_CLK = 120*1000*1000; // 240 MHz/2
#elif defined ( CONFIG_IDF_TARGET_ESP32P4 )
    static constexpr uint32_t PLL_D2_CLK = 20*1000*1000; // 20 MHz
#else
    static constexpr uint32_t PLL_D2_CLK = 80*1000*1000; // 160 MHz/2
#endif

    uint32_t bits = (self->_cfg.use_adc) ? 1 : 16; /// 1サンプリング当たりの出力ビット数;
    uint32_t div_a, div_b, div_n;

    // CoreS3 のマイクはmclkの倍率(div_m)の値を8以上に設定しないと精度が落ちる。
    uint32_t div_m = 8;

    // PDM録音時、DSR(データサンプリングレート) 64に設定する
    if (use_pdm) { bits = 64; div_m = 2; }
    calcClockDiv(&div_a, &div_b, &div_n, PLL_D2_CLK / (bits * div_m), self->_cfg.sample_rate * oversampling);

    auto dev = &I2S0;
#if SOC_I2S_NUM >= 2
    if (self->_cfg.i2s_port == i2s_port_t::I2S_NUM_1) { dev = &I2S1; }
#if SOC_I2S_NUM >= 3
    else if (self->_cfg.i2s_port == i2s_port_t::I2S_NUM_2) { dev = &I2S2; }
#if SOC_I2S_NUM >= 4
    else if (self->_cfg.i2s_port == i2s_port_t::I2S_NUM_3) { dev = &I2S3; }
#endif
#endif
#endif

#if defined ( CONFIG_IDF_TARGET_ESP32C3 ) || defined ( CONFIG_IDF_TARGET_ESP32C6 ) || defined ( CONFIG_IDF_TARGET_ESP32S3 ) || defined ( CONFIG_IDF_TARGET_ESP32P4 )

    dev->rx_conf.rx_pdm_en = use_pdm;
    dev->rx_conf.rx_tdm_en = !use_pdm;
#if defined ( I2S_RX_PDM2PCM_CONF_REG )
    dev->rx_pdm2pcm_conf.rx_pdm2pcm_en = use_pdm;
    dev->rx_pdm2pcm_conf.rx_pdm_sinc_dsr_16_en = 0;
#elif defined (I2S_RX_PDM2PCM_EN)
    dev->rx_conf.rx_pdm2pcm_en = use_pdm;
    dev->rx_conf.rx_pdm_sinc_dsr_16_en = 0;
#endif
    dev->rx_conf.rx_update = 1;

#if defined ( CONFIG_IDF_TARGET_ESP32P4 )
    dev->rx_conf.rx_bck_div_num = div_m - 1;
#else
    dev->rx_conf1.rx_bck_div_num = div_m - 1;
#endif

    bool yn1 = (div_b > (div_a >> 1));
    if (yn1) {
      div_b = div_a - div_b;
    }
    int div_y = 1;
    int div_x = 0;
    if (div_b)
    {
      div_x = div_a / div_b - 1;
      div_y = div_a % div_b;

      if (div_y == 0)
      { // div_yが0になる場合、分数成分が無視される不具合があり、
        // 指定よりクロックが速くなってしまう。
        // 回避策として、誤差が少なくなる設定値を導入する。
        // これにより、誤差をクロック周期512回に1回程度のズレに抑える。;
        div_y = 1;
        div_b = 511;
      }
    }

#if __has_include(<driver/i2s_std.h>)
    i2s_ll_rx_set_raw_clk_div(dev, div_n, div_x, div_y, div_b, yn1);
#endif

#if __has_include (<soc/pcr_struct.h>) // for C6
    PCR.i2s_rx_clkm_div_conf.i2s_rx_clkm_div_x = div_x;
    PCR.i2s_rx_clkm_div_conf.i2s_rx_clkm_div_y = div_y;
    PCR.i2s_rx_clkm_div_conf.i2s_rx_clkm_div_z = div_b;
    PCR.i2s_rx_clkm_div_conf.i2s_rx_clkm_div_yn1 = yn1;
    PCR.i2s_rx_clkm_conf.i2s_rx_clkm_div_num = div_n;
    PCR.i2s_rx_clkm_conf.i2s_rx_clkm_sel = 1;   // PLL_240M_CLK
    PCR.i2s_rx_clkm_conf.i2s_rx_clkm_en = 1;
    PCR.pll_div_clk_en.pll_240m_clk_en = 1;
#elif defined ( I2S_RX_CLKM_DIV_X )
    dev->rx_clkm_div_conf.rx_clkm_div_x = div_x;
    dev->rx_clkm_div_conf.rx_clkm_div_y = div_y;
    dev->rx_clkm_div_conf.rx_clkm_div_z = div_b;
    dev->rx_clkm_div_conf.rx_clkm_div_yn1 = yn1;
    dev->rx_clkm_conf.rx_clkm_div_num = div_n;
    dev->rx_clkm_conf.rx_clk_sel = 1;   // PLL_240M_CLK
    dev->tx_clkm_conf.clk_en = 1;
    dev->rx_clkm_conf.rx_clk_active = 1;

    dev->rx_conf.rx_update = 1;
    dev->rx_conf.rx_update = 0;
#endif

#else

    if (use_pdm)
    {
      dev->pdm_conf.rx_sinc_dsr_16_en = 1; // 0=DSR64 / 1=DSR128
      dev->pdm_conf.pdm2pcm_conv_en = 1;
      dev->pdm_conf.rx_pdm_en = 1;
    }

    dev->sample_rate_conf.rx_bck_div_num = div_m;
    dev->clkm_conf.clkm_div_a = div_a;
    dev->clkm_conf.clkm_div_b = div_b;
    dev->clkm_conf.clkm_div_num = div_n;
    dev->clkm_conf.clka_en = 0; // APLL disable : PLL_160M

    // If RX is not reset here, BCK polarity may be inverted.
    dev->conf.rx_reset = 1;
    dev->conf.rx_fifo_reset = 1;
    dev->conf.rx_reset = 0;
    dev->conf.rx_fifo_reset = 0;

#endif

    _i2s_start(self->_cfg.i2s_port);

    int32_t gain = self->_cfg.magnification;
    const float f_gain = (float)gain / (oversampling << 1);
    size_t src_idx = ~0u;
    size_t src_len = 0;
    int32_t sum_value[4] = { 0,0 };
    int32_t prev_value[2] = { 0, 0 };
    const bool in_stereo = self->_cfg.stereo;
    int32_t os_remain = oversampling;
    const size_t dma_buf_len = self->_cfg.dma_buf_len;
    int16_t* src_buf = (int16_t*)alloca(dma_buf_len * sizeof(int16_t));
    memset(src_buf, 0, dma_buf_len * sizeof(int16_t));

    _i2s_read(self->_cfg.i2s_port, src_buf, dma_buf_len, &src_len, portTICK_PERIOD_MS);
    _i2s_read(self->_cfg.i2s_port, src_buf, dma_buf_len, &src_len, portTICK_PERIOD_MS);

    while (self->_task_running)
    {
      bool rec_flip = self->_rec_flip;
      recording_info_t* current_rec = &(self->_rec_info[!rec_flip]);
      recording_info_t* next_rec    = &(self->_rec_info[ rec_flip]);

      size_t dst_remain = current_rec->length;
      if (dst_remain == 0)
      {
        rec_flip = !rec_flip;
        self->_rec_flip = rec_flip;
        xSemaphoreGive(self->_task_semaphore);
        std::swap(current_rec, next_rec);
        dst_remain = current_rec->length;
        if (dst_remain == 0)
        {
          self->_is_recording = false;
          ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
          src_idx = ~0u;
          src_len = 0;
          sum_value[0] = 0;
          sum_value[1] = 0;
          continue;
        }
      }
      self->_is_recording = true;

      for (;;)
      {
        if (src_idx >= src_len)
        {
          _i2s_read(self->_cfg.i2s_port, src_buf, dma_buf_len, &src_len, 100 / portTICK_PERIOD_MS);
          src_len >>= 1;
          src_idx = 0;
        }

        do
        {
          sum_value[0] += src_buf[src_idx  ];
          sum_value[1] += src_buf[src_idx+1];
          src_idx += 2;
        } while (--os_remain && (src_idx < src_len));

        if (os_remain) { continue; }
        os_remain = oversampling;

#if defined (CONFIG_IDF_TARGET_ESP32)
        auto sv0 = sum_value[1];
        auto sv1 = sum_value[0];
#else
        auto sv0 = sum_value[0];
        auto sv1 = sum_value[1];
#endif
        if (self->_cfg.use_adc) {
          sv0 -= 2048 * oversampling;
          sv1 -= 2048 * oversampling;
        }

        auto value_tmp = (sv0 + sv1) << 3;
        int32_t offset = self->_offset;
        // Automatic zero level adjustment
        offset -= (value_tmp + offset + 16) >> 5;
        self->_offset = offset;
        offset = (offset + 8) >> 4;
        sum_value[0] = sv0 + offset;
        sum_value[1] = sv1 + offset;

        int32_t noise_filter = self->_cfg.noise_filter_level;
        if (noise_filter)
        {
          for (int i = 0; i < 2; ++i)
          {
            int32_t v = (sum_value[i] * (256 - noise_filter) + prev_value[i] * noise_filter + 128) >> 8;
            prev_value[i] = v;
            sum_value[i] = v * f_gain;
          }
        }
        else
        {
          for (int i = 0; i < 2; ++i)
          {
            sum_value[i] *= f_gain;
          }
        }

        int output_num = 2;

        if (in_stereo != current_rec->is_stereo)
        {
          if (in_stereo)
          { // stereo -> mono  convert.
            sum_value[0] = (sum_value[0] + sum_value[1] + 1) >> 1;
            output_num = 1;
          }
          else
          { // mono -> stereo  convert.
            auto tmp = sum_value[1];
            sum_value[3] = tmp;
            sum_value[2] = tmp;
            sum_value[1] = sum_value[0];
            output_num = 4;
          }
        }
        for (int i = 0; i < output_num; ++i)
        {
          auto value = sum_value[i];
          if (current_rec->is_16bit)
          {
            if (     value < INT16_MIN+16) { value = INT16_MIN+16; }
            else if (value > INT16_MAX-16) { value = INT16_MAX-16; }
            auto dst = (int16_t*)(current_rec->data);
            *dst++ = value;
            current_rec->data = dst;
          }
          else
          {
            value = ((value + 128) >> 8) + 128;
            if (     value < 0) { value = 0; }
            else if (value > 255) { value = 255; }
            auto dst = (uint8_t*)(current_rec->data);
            *dst++ = value;
            current_rec->data = dst;
          }
        }
        sum_value[0] = 0;
        sum_value[1] = 0;
        dst_remain -= output_num;
        if ((int32_t)dst_remain <= 0)
        {
          current_rec->length = 0;
          break;
        }
      }
    }
    self->_is_recording = false;
    _i2s_stop(self->_cfg.i2s_port);

    self->_task_handle = nullptr;
    vTaskDelete(nullptr);
  }

  bool Mic_Class::begin(void)
  {
    if (_task_running)
    {
      auto rate = _calc_rec_rate();
      if (_rec_sample_rate == rate)
      {
        return true;
      }
      do { vTaskDelay(1); } while (isRecording());
      end();
      _rec_sample_rate = rate;
    }

    if (_task_semaphore == nullptr) { _task_semaphore = xSemaphoreCreateBinary(); }

    bool res = true;
    if (_cb_set_enabled) { res = _cb_set_enabled(_cb_set_enabled_args, true); }

    res = (ESP_OK == _setup_i2s()) && res;
    if (res)
    {
      size_t stack_size = 2048 + (_cfg.dma_buf_len * sizeof(uint16_t));
      _task_running = true;
#if portNUM_PROCESSORS > 1
      if (_cfg.task_pinned_core < portNUM_PROCESSORS)
      {
        xTaskCreatePinnedToCore(mic_task, "mic_task", stack_size, this, _cfg.task_priority, &_task_handle, _cfg.task_pinned_core);
      }
      else
#endif
      {
        xTaskCreate(mic_task, "mic_task", stack_size, this, _cfg.task_priority, &_task_handle);
      }
    }

    return res;
  }

  void Mic_Class::end(void)
  {
    if (!_task_running) { return; }
    _task_running = false;
    if (_task_handle)
    {
      if (_task_handle) { xTaskNotifyGive(_task_handle); }
      do { vTaskDelay(1); } while (_task_handle);
    }

    if (_cb_set_enabled) { _cb_set_enabled(_cb_set_enabled_args, false); }
    _i2s_driver_uninstall(_cfg.i2s_port);
  }

  bool Mic_Class::_rec_raw(void* recdata, size_t array_len, bool flg_16bit, uint32_t sample_rate, bool flg_stereo)
  {
    recording_info_t info;
    info.data = recdata;
    info.length = array_len;
    info.is_16bit = flg_16bit;
    info.is_stereo = flg_stereo;

    _cfg.sample_rate = sample_rate;

    if (!begin()) { return false; }
    if (array_len == 0) { return true; }
    while (_rec_info[_rec_flip].length) { xSemaphoreTake(_task_semaphore, 1); }
    _rec_info[_rec_flip] = info;
    if (this->_task_handle)
    {
      xTaskNotifyGive(this->_task_handle);
    }
    return true;
  }
#endif
}
#endif
