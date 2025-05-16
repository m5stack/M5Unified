// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include "M5Unified.hpp"
#include "utility/PI4IOE5V6408_Class.hpp"

#if !defined (M5UNIFIED_PC_BUILD)
#include <soc/efuse_reg.h>
#include <soc/gpio_periph.h>

#if !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
 #if __has_include (<driver/touch_sens.h>)
 #include <driver/touch_sens.h>
 #elif __has_include (<driver/touch_sensor.h>)
 #include <driver/touch_sensor.h>
 #endif
#endif

#if __has_include (<driver/i2s_type.h>)
#include <driver/i2s_type.h>
#endif

#if __has_include (<esp_idf_version.h>)
 #include <esp_idf_version.h>
 #if ESP_IDF_VERSION_MAJOR >= 4
  /// [[fallthrough]];
  #define NON_BREAK ;[[fallthrough]];
 #endif

#endif
#endif

/// [[fallthrough]];
#ifndef NON_BREAK
#define NON_BREAK ;
#endif

/// global instance.
m5::M5Unified M5;

void __attribute((weak)) adc_power_acquire(void)
{
#if !defined (M5UNIFIED_PC_BUILD)
#if defined (ESP_IDF_VERSION_VAL)
 #if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(3, 3, 4)
  adc_power_on();
 #endif
#else
 adc_power_on();
#endif
#endif
}

namespace m5
{
int8_t M5Unified::_get_pin_table[pin_name_max];

#if defined (M5UNIFIED_PC_BUILD)
  void M5Unified::_setup_pinmap(board_t)
  {
    std::fill(_get_pin_table, _get_pin_table + pin_name_max, 255);
  }
#else
// ピン番号テーブル。 unknownをテーブルの最後に配置する。該当が無い場合はunknownの値が使用される。
static constexpr const uint8_t _pin_table_i2c_ex_in[][5] = {
                            // In CL,DA, EX CL,DA
#if defined (CONFIG_IDF_TARGET_ESP32S3)
{ board_t::board_M5StackCoreS3, GPIO_NUM_11,GPIO_NUM_12 , GPIO_NUM_1 ,GPIO_NUM_2  },
{ board_t::board_M5StackCoreS3SE,GPIO_NUM_11,GPIO_NUM_12, GPIO_NUM_1 ,GPIO_NUM_2  },
{ board_t::board_M5StampS3    , 255        ,255         , GPIO_NUM_15,GPIO_NUM_13 },
{ board_t::board_M5Capsule    , GPIO_NUM_10,GPIO_NUM_8  , GPIO_NUM_15,GPIO_NUM_13 },
{ board_t::board_M5Dial       , GPIO_NUM_12,GPIO_NUM_11 , GPIO_NUM_15,GPIO_NUM_13 },
{ board_t::board_M5DinMeter   , GPIO_NUM_12,GPIO_NUM_11 , GPIO_NUM_15,GPIO_NUM_13 },
{ board_t::board_M5AirQ       , GPIO_NUM_12,GPIO_NUM_11 , GPIO_NUM_15,GPIO_NUM_13 },
{ board_t::board_M5Cardputer  , 255        ,255         , GPIO_NUM_1 ,GPIO_NUM_2  },
{ board_t::board_M5VAMeter    , GPIO_NUM_6 ,GPIO_NUM_5  , GPIO_NUM_9 ,GPIO_NUM_8  },
{ board_t::board_M5AtomS3R    , GPIO_NUM_0 ,GPIO_NUM_45 , GPIO_NUM_1 ,GPIO_NUM_2  },
{ board_t::board_M5AtomS3RExt , GPIO_NUM_0 ,GPIO_NUM_45 , GPIO_NUM_1 ,GPIO_NUM_2  },
{ board_t::board_M5AtomS3RCam , GPIO_NUM_0 ,GPIO_NUM_45 , GPIO_NUM_1 ,GPIO_NUM_2  },
{ board_t::board_M5PaperS3    , GPIO_NUM_42,GPIO_NUM_41 , GPIO_NUM_1 ,GPIO_NUM_2  },
{ board_t::board_M5StampPLC   , GPIO_NUM_15,GPIO_NUM_13 , GPIO_NUM_1 ,GPIO_NUM_2  },
{ board_t::board_unknown      , GPIO_NUM_39,GPIO_NUM_38 , GPIO_NUM_1 ,GPIO_NUM_2  }, // AtomS3,AtomS3Lite,AtomS3U
#elif defined (CONFIG_IDF_TARGET_ESP32C3)
{ board_t::board_unknown      , 255        ,255         , GPIO_NUM_0 ,GPIO_NUM_1  },
#elif defined (CONFIG_IDF_TARGET_ESP32C6)
{ board_t::board_unknown      , 255        ,255         , GPIO_NUM_1 ,GPIO_NUM_2  }, // NanoC6
#elif defined (CONFIG_IDF_TARGET_ESP32P4)
{ board_t::board_M5Tab5       , GPIO_NUM_32,GPIO_NUM_31, GPIO_NUM_54,GPIO_NUM_53 }, // Tab5
{ board_t::board_unknown      , 255        ,255         , 255        ,255        },
#else
{ board_t::board_M5Stack      , GPIO_NUM_22,GPIO_NUM_21 , GPIO_NUM_22,GPIO_NUM_21 },
{ board_t::board_M5Paper      , GPIO_NUM_22,GPIO_NUM_21 , GPIO_NUM_32,GPIO_NUM_25 },
{ board_t::board_M5TimerCam   , GPIO_NUM_14,GPIO_NUM_12 , GPIO_NUM_13,GPIO_NUM_4  },
{ board_t::board_M5AtomLite   , GPIO_NUM_21,GPIO_NUM_25 , GPIO_NUM_32,GPIO_NUM_26 },
{ board_t::board_M5AtomMatrix , GPIO_NUM_21,GPIO_NUM_25 , GPIO_NUM_32,GPIO_NUM_26 },
{ board_t::board_M5AtomEcho   , GPIO_NUM_21,GPIO_NUM_25 , GPIO_NUM_32,GPIO_NUM_26 },
{ board_t::board_M5AtomU      , GPIO_NUM_21,GPIO_NUM_25 , GPIO_NUM_32,GPIO_NUM_26 },
{ board_t::board_M5AtomPsram  , GPIO_NUM_21,GPIO_NUM_25 , GPIO_NUM_32,GPIO_NUM_26 },
{ board_t::board_unknown      , GPIO_NUM_22,GPIO_NUM_21 , GPIO_NUM_33,GPIO_NUM_32 }, // Core2,Tough,StickC,CoreInk,Station,StampPico
#endif
};

static constexpr const uint8_t _pin_table_port_bc[][5] = {
                          //pB p1,p2, pC p1,p2
#if defined (CONFIG_IDF_TARGET_ESP32S3)
{ board_t::board_M5StackCoreS3, GPIO_NUM_8 ,GPIO_NUM_9 , GPIO_NUM_18,GPIO_NUM_17 },
{ board_t::board_M5StackCoreS3SE,GPIO_NUM_8,GPIO_NUM_9 , GPIO_NUM_18,GPIO_NUM_17 },
{ board_t::board_M5Dial       , GPIO_NUM_1 ,GPIO_NUM_2 , 255        ,255         },
{ board_t::board_M5DinMeter   , GPIO_NUM_1 ,GPIO_NUM_2 , 255        ,255         },
#elif defined (CONFIG_IDF_TARGET_ESP32C3)
#elif defined (CONFIG_IDF_TARGET_ESP32C6)
#elif defined (CONFIG_IDF_TARGET_ESP32P4)
{ board_t::board_M5Tab5       , GPIO_NUM_17,GPIO_NUM_52, GPIO_NUM_7 ,GPIO_NUM_6  }, // Tab5
#else
{ board_t::board_M5Stack      , GPIO_NUM_36,GPIO_NUM_26 , GPIO_NUM_16,GPIO_NUM_17 },
{ board_t::board_M5StackCore2 , GPIO_NUM_36,GPIO_NUM_26 , GPIO_NUM_13,GPIO_NUM_14 },
{ board_t::board_M5Paper      , GPIO_NUM_33,GPIO_NUM_26 , GPIO_NUM_19,GPIO_NUM_18 },
{ board_t::board_M5Station    , GPIO_NUM_35,GPIO_NUM_25 , GPIO_NUM_13,GPIO_NUM_14 },
#endif
{ board_t::board_unknown      , 255        ,255         , 255        ,255 },
};

static constexpr const uint8_t _pin_table_port_de[][5] = {
                          //pD p1,p2, pE p1,p2
#if defined (CONFIG_IDF_TARGET_ESP32S3)
{ board_t::board_M5StackCoreS3, 14,10, 18,17 },
{ board_t::board_M5StackCoreS3SE,14,10,18,17 },
#elif defined (CONFIG_IDF_TARGET_ESP32C3)
#elif defined (CONFIG_IDF_TARGET_ESP32C6)
#else
{ board_t::board_M5Stack      , GPIO_NUM_34,GPIO_NUM_35 , GPIO_NUM_5 ,GPIO_NUM_13 },
{ board_t::board_M5StackCore2 , GPIO_NUM_34,GPIO_NUM_35 , GPIO_NUM_27,GPIO_NUM_19 },
{ board_t::board_M5Station    , GPIO_NUM_36,GPIO_NUM_26 , GPIO_NUM_16,GPIO_NUM_17 }, // B2 / C2
#endif
{ board_t::board_unknown      , 255        ,255         , 255        ,255         },
};

static constexpr const uint8_t _pin_table_spi_sd[][5] = {
                            // clk,mosi,miso,cs
#if defined (CONFIG_IDF_TARGET_ESP32S3)
{ board_t::board_M5StackCoreS3, GPIO_NUM_36, GPIO_NUM_37, GPIO_NUM_35, GPIO_NUM_4  },
{ board_t::board_M5StackCoreS3SE,GPIO_NUM_36,GPIO_NUM_37, GPIO_NUM_35, GPIO_NUM_4  },
{ board_t::board_M5Capsule    , GPIO_NUM_14, GPIO_NUM_12, GPIO_NUM_39, GPIO_NUM_11 },
{ board_t::board_M5Cardputer  , GPIO_NUM_40, GPIO_NUM_14, GPIO_NUM_39, GPIO_NUM_12 },
{ board_t::board_M5PaperS3    , GPIO_NUM_39, GPIO_NUM_38, GPIO_NUM_40, GPIO_NUM_47 },
{ board_t::board_M5StampPLC   , GPIO_NUM_7,  GPIO_NUM_8,  GPIO_NUM_9,  GPIO_NUM_10 },
#elif defined (CONFIG_IDF_TARGET_ESP32C3)
#elif defined (CONFIG_IDF_TARGET_ESP32C6)
#elif defined (CONFIG_IDF_TARGET_ESP32P4)
{ board_t::board_M5Tab5       , GPIO_NUM_43,GPIO_NUM_44, GPIO_NUM_39, GPIO_NUM_42 },
#else
{ board_t::board_M5Stack      , GPIO_NUM_18, GPIO_NUM_23, GPIO_NUM_19, GPIO_NUM_4  },
{ board_t::board_M5StackCore2 , GPIO_NUM_18, GPIO_NUM_23, GPIO_NUM_38, GPIO_NUM_4  },
{ board_t::board_M5Paper      , GPIO_NUM_14, GPIO_NUM_12, GPIO_NUM_13, GPIO_NUM_4  },
#endif
{ board_t::board_unknown      , 255        , 255        , 255        , 255         },
};

static constexpr const uint8_t _pin_table_other0[][2] = {
                             //RGBLED
#if defined (CONFIG_IDF_TARGET_ESP32S3)
{ board_t::board_M5AtomS3U    , GPIO_NUM_35 },
{ board_t::board_M5AtomS3Lite , GPIO_NUM_35 },
{ board_t::board_M5StampS3    , GPIO_NUM_21 },
{ board_t::board_M5StampPLC   , GPIO_NUM_21 },
{ board_t::board_M5AirQ       , GPIO_NUM_21 },
{ board_t::board_M5Dial       , GPIO_NUM_21 },
{ board_t::board_M5DinMeter   , GPIO_NUM_21 },
{ board_t::board_M5Capsule    , GPIO_NUM_21 },
{ board_t::board_M5Cardputer  , GPIO_NUM_21 },
#elif defined (CONFIG_IDF_TARGET_ESP32C3)
{ board_t::board_M5StampC3    , GPIO_NUM_2  },
{ board_t::board_M5StampC3U   , GPIO_NUM_2  },
#elif defined (CONFIG_IDF_TARGET_ESP32C6)
{ board_t::board_M5NanoC6     , GPIO_NUM_20 },
#else
{ board_t::board_M5Stack      , GPIO_NUM_15 },
{ board_t::board_M5StackCore2 , GPIO_NUM_25 },
{ board_t::board_M5Station    , GPIO_NUM_4  },
{ board_t::board_M5AtomLite   , GPIO_NUM_27 },
{ board_t::board_M5AtomMatrix , GPIO_NUM_27 },
{ board_t::board_M5AtomEcho   , GPIO_NUM_27 },
{ board_t::board_M5AtomU      , GPIO_NUM_27 },
{ board_t::board_M5AtomPsram  , GPIO_NUM_27 },
{ board_t::board_M5StampPico  , GPIO_NUM_27 },
#endif
{ board_t::board_unknown      , 255         },
};

static constexpr const uint8_t _pin_table_other1[][2] = {
                             //POWER_HOLD
#if defined (CONFIG_IDF_TARGET_ESP32S3)
{ board_t::board_M5Dial        , GPIO_NUM_46 },
{ board_t::board_M5Capsule     , GPIO_NUM_46 },
{ board_t::board_M5AirQ        , GPIO_NUM_46 },
{ board_t::board_M5DinMeter    , GPIO_NUM_46 },
{ board_t::board_M5PaperS3     , GPIO_NUM_44 },

#elif defined (CONFIG_IDF_TARGET_ESP32C3)
#elif defined (CONFIG_IDF_TARGET_ESP32C6)
#else

{ board_t::board_M5StickCPlus2 , GPIO_NUM_4  },
{ board_t::board_M5Paper       , GPIO_NUM_2  },
{ board_t::board_M5StackCoreInk, GPIO_NUM_12 },
{ board_t::board_M5TimerCam    , GPIO_NUM_33 },

#endif
{ board_t::board_unknown      , 255         },
};

static constexpr const uint8_t _pin_table_mbus[][31] = {
#if defined (CONFIG_IDF_TARGET_ESP32P4)
{ board_t::board_M5Tab5   ,
  255        , GPIO_NUM_16,
  255        , GPIO_NUM_17,
  255        , 255        ,
  GPIO_NUM_18, GPIO_NUM_45,
  GPIO_NUM_19, GPIO_NUM_52,
  GPIO_NUM_5 , 255        ,
  GPIO_NUM_38, GPIO_NUM_37,
  GPIO_NUM_7 , GPIO_NUM_6 ,
  GPIO_NUM_31, GPIO_NUM_32,
  GPIO_NUM_3 , GPIO_NUM_4 ,
  GPIO_NUM_2 , GPIO_NUM_48,
  GPIO_NUM_47, GPIO_NUM_35,
  255        , GPIO_NUM_51,
  255        , 255        ,
  255        , 255        ,
},
#elif defined (CONFIG_IDF_TARGET_ESP32S3)
{ board_t::board_M5StackCoreS3,
  255        , GPIO_NUM_10,
  255        , GPIO_NUM_8 ,
  255        , 255        ,
  GPIO_NUM_37, GPIO_NUM_5 ,
  GPIO_NUM_35, GPIO_NUM_9 ,
  GPIO_NUM_36, 255        ,
  GPIO_NUM_44, GPIO_NUM_43,
  GPIO_NUM_18, GPIO_NUM_17,
  GPIO_NUM_12, GPIO_NUM_11,
  GPIO_NUM_2 , GPIO_NUM_1 ,
  GPIO_NUM_6 , GPIO_NUM_7 ,
  GPIO_NUM_13, GPIO_NUM_0 ,
  255        , GPIO_NUM_14,
  255        , 255        ,
  255        , 255        ,
},
{ board_t::board_M5StackCoreS3SE,
  255        , GPIO_NUM_10,
  255        , GPIO_NUM_8 ,
  255        , 255        ,
  GPIO_NUM_37, GPIO_NUM_5 ,
  GPIO_NUM_35, GPIO_NUM_9 ,
  GPIO_NUM_36, 255        ,
  GPIO_NUM_44, GPIO_NUM_43,
  GPIO_NUM_18, GPIO_NUM_17,
  GPIO_NUM_12, GPIO_NUM_11,
  GPIO_NUM_2 , GPIO_NUM_1 ,
  GPIO_NUM_6 , GPIO_NUM_7 ,
  GPIO_NUM_13, GPIO_NUM_0 ,
  255        , GPIO_NUM_14,
  255        , 255        ,
  255        , 255        ,
},
#elif defined (CONFIG_IDF_TARGET_ESP32C3)
#elif defined (CONFIG_IDF_TARGET_ESP32C6)
#else
{ board_t::board_M5Stack  ,
  255        , GPIO_NUM_35,
  255        , GPIO_NUM_36,
  255        , 255        ,
  GPIO_NUM_23, GPIO_NUM_25,
  GPIO_NUM_19, GPIO_NUM_26,
  GPIO_NUM_18, 255        ,
  GPIO_NUM_3 , GPIO_NUM_1 ,
  GPIO_NUM_16, GPIO_NUM_17,
  GPIO_NUM_21, GPIO_NUM_22,
  GPIO_NUM_2 , GPIO_NUM_5 ,
  GPIO_NUM_12, GPIO_NUM_13,
  GPIO_NUM_15, GPIO_NUM_0 ,
  255        , GPIO_NUM_34,
  255        , 255        ,
  255        , 255        ,
},
{ board_t::board_M5StackCore2,
  255        , GPIO_NUM_35,
  255        , GPIO_NUM_36,
  255        , 255        ,
  GPIO_NUM_23, GPIO_NUM_25,
  GPIO_NUM_38, GPIO_NUM_26,
  GPIO_NUM_18, 255        ,
  GPIO_NUM_3 , GPIO_NUM_1 ,
  GPIO_NUM_13, GPIO_NUM_14,
  GPIO_NUM_21, GPIO_NUM_22,
  GPIO_NUM_32, GPIO_NUM_33,
  GPIO_NUM_27, GPIO_NUM_19,
  GPIO_NUM_2 , GPIO_NUM_0 ,
  255        , GPIO_NUM_34,
  255        , 255        ,
  255        , 255        ,
},
#endif
{ board_t::board_unknown  , 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
};

  void M5Unified::_setup_pinmap(board_t id)
  {
    constexpr const std::pair<const void*, size_t> tbl[] = {
      { _pin_table_i2c_ex_in, sizeof(_pin_table_i2c_ex_in[0]) },
      { _pin_table_port_bc, sizeof(_pin_table_port_bc[0]) },
      { _pin_table_port_de, sizeof(_pin_table_port_de[0]) },
      { _pin_table_spi_sd, sizeof(_pin_table_spi_sd[0]) },
      { _pin_table_other0, sizeof(_pin_table_other0[0]) },
      { _pin_table_other1, sizeof(_pin_table_other1[0]) },
      { _pin_table_mbus, sizeof(_pin_table_mbus[0]) },
    };

    int8_t* dst = _get_pin_table;
    for (auto &p : tbl) {
      const uint8_t* t = (uint8_t*)p.first;
      size_t len = p.second;
      while (t[0] != id && t[0] != board_t::board_unknown) { t += len; }
      memcpy(dst, &t[1], len - 1);
      dst += len - 1;
    }
  }
#endif

  static PI4IOE5V6408_Class* _io_expander_a = nullptr;
  PI4IOE5V6408_Class* __get_io_expander_a()
  {
    return _io_expander_a;
  }

  void _io_expander_a_init()
  {
    // Init
    _io_expander_a = new PI4IOE5V6408_Class;
    if (!_io_expander_a->begin()) {
      delete _io_expander_a;
      _io_expander_a = nullptr;
    } else {
      _io_expander_a->resetIrq();
    }
  }

  static void in_i2c_bulk_write(const uint8_t i2c_addr, const uint8_t* bulk_data, const uint32_t i2c_freq = 100000u, const uint8_t retry = 0)
  {
    // bulk_data example..
    // const uint8_t bulk_data[] = {
    //   2, 0x00, 0x00,       // <- datalen = 2, reg = 0x00, data = 0x00
    //   3, 0x01, 0x00, 0x02, // <- datalen = 3, reg = 0x01, data = 0x00, 0x02
    //   0 };                 // <- datalen 0 is end of data.

    while (*bulk_data) {
      uint8_t len = *bulk_data++;
      uint8_t r = retry + 1;
      while (!M5.In_I2C.writeRegister(i2c_addr, bulk_data[0], &bulk_data[1], len - 1, i2c_freq) && --r) { M5.delay(1); }
      bulk_data += len;
    }
  }

  static constexpr uint8_t es7210_i2c_addr = 0x40;
  static constexpr uint8_t es8311_i2c_addr0 = 0x18;
  static constexpr uint8_t es8311_i2c_addr1 = 0x19;
  static constexpr uint8_t es8388_i2c_addr = 0x10;
  static constexpr uint8_t pi4io1_i2c_addr = 0x43;
#if defined (CONFIG_IDF_TARGET_ESP32S3)
  static constexpr uint8_t aw88298_i2c_addr = 0x36;
  static constexpr uint8_t aw9523_i2c_addr = 0x58;
  static void aw88298_write_reg(uint8_t reg, uint16_t value)
  {
    value = __builtin_bswap16(value);
    M5.In_I2C.writeRegister(aw88298_i2c_addr, reg, (const uint8_t*)&value, 2, 400000);
  }

  static void es7210_write_reg(uint8_t reg, uint8_t value)
  {
    M5.In_I2C.writeRegister(es7210_i2c_addr, reg, &value, 1, 400000);
  }

#endif

  bool M5Unified::_speaker_enabled_cb_core2(void* args, bool enabled)
  {
    (void)args;
    (void)enabled;
#if defined (CONFIG_IDF_TARGET_ESP32)
    auto self = (M5Unified*)args;
    auto spk_cfg = self->Speaker.config();
    if (spk_cfg.pin_bck      == GPIO_NUM_12
      && spk_cfg.pin_ws       == GPIO_NUM_0
      && spk_cfg.pin_data_out == GPIO_NUM_2
    ) {
      switch (self->Power.getType()) {
      case m5::Power_Class::pmic_axp192:
        self->Power.Axp192.setGPIO2(enabled);
        break;
      case m5::Power_Class::pmic_axp2101:
        self->Power.Axp2101.setALDO3(enabled * 3300);
        break;
      default:
        break;
      }
    }
#endif
    return true;
  }

  bool M5Unified::_speaker_enabled_cb_cores3(void* args, bool enabled)
  {
    (void)args;
    (void)enabled;
#if defined (CONFIG_IDF_TARGET_ESP32S3)
    auto self = (M5Unified*)args;
    auto spk_cfg = self->Speaker.config();
    if (spk_cfg.pin_bck == GPIO_NUM_34 && enabled)
    {
      self->In_I2C.bitOn(aw9523_i2c_addr, 0x02, 0b00000100, 400000);
      /// サンプリングレートに応じてAW88298のレジスタの設定値を変える;
      static constexpr uint8_t rate_tbl[] = {4,5,6,8,10,11,15,20,22,44};
      size_t reg0x06_value = 0;
      size_t rate = (spk_cfg.sample_rate + 1102) / 2205;
      while (rate > rate_tbl[reg0x06_value] && ++reg0x06_value < sizeof(rate_tbl)) {}

      reg0x06_value |= 0x14C0;  // I2SBCK=0 (BCK mode 16*2)
      aw88298_write_reg( 0x61, 0x0673 );  // boost mode disabled 
      aw88298_write_reg( 0x04, 0x4040 );  // I2SEN=1 AMPPD=0 PWDN=0
      aw88298_write_reg( 0x05, 0x0008 );  // RMSE=0 HAGCE=0 HDCCE=0 HMUTE=0
      aw88298_write_reg( 0x06, reg0x06_value );
      aw88298_write_reg( 0x0C, 0x0064 );  // volume setting (full volume)
    }
    else /// disableにする場合および内蔵スピーカ以外を操作対象とした場合、内蔵スピーカを停止する。
    {
      aw88298_write_reg( 0x04, 0x4000 );  // I2SEN=0 AMPPD=0 PWDN=0
      self->In_I2C.bitOff(aw9523_i2c_addr, 0x02, 0b00000100, 400000);
    }
#endif
    return true;
  }

  bool M5Unified::_speaker_enabled_cb_tab5(void* args, bool enabled)
  {
    (void)args;
    (void)enabled;
#if defined (CONFIG_IDF_TARGET_ESP32P4)
    auto self = (M5Unified*)args;
    auto spk_cfg = self->Speaker.config();
    if (spk_cfg.pin_data_out != GPIO_NUM_26) { return false; }

    static constexpr const uint8_t enabled_bulk_data[] = {
      2,    0, 0x80,  // RESET/  CSM POWER ON
      2,    0, 0x00,
      2,    0, 0x00,
      2,    0, 0x0E,
      2,    1, 0x00,
      2,    2, 0x0A, //CHIP POWER: power up all
      2,    3, 0xFF, //ADC POWER: power down all
      2,    4, 0x3C, //DAC POWER: power up and LOUT1/ROUT1/LOUT2/ROUT2 enable
      2,    5, 0x00, //ChipLowPower1
      2,    6, 0x00, //ChipLowPower2
      2,    7, 0x7C, //VSEL
      2,    8, 0x00, //set I2S slave mode
      // reg9-22 == adc
      2,   23, 0x18, //I2S format (16bit)
      2,   24, 0x00, //I2S MCLK ratio (128)
      2,   25, 0x20, //DAC unmute
      2,   26, 0x00, //LDACVOL 0x00~0xC0
      2,   27, 0x00, //RDACVOL 0x00~0xC0
      2,   28, 0x08, //enable digital click free power up and down
      2,   29, 0x00,
      2,   38, 0x00, //DAC CTRL16
      2,   39, 0xB8, //LEFT Ch MIX
      2,   42, 0xB8, //RIGHTCh MIX
      2,   43, 0x08, //ADC and DAC separate
      2,   45, 0x00, // 0x00=1.5k VREF analog output / 0x10=40kVREF analog output
      2,   46, 0x21,
      2,   47, 0x21,
      2,   48, 0x21,
      2,   49, 0x21,
      0
    };
    static constexpr const uint8_t disabled_bulk_data[] = {
      2,    8, 0x00, //set I2S slave mode
      0
    };
    in_i2c_bulk_write(es8388_i2c_addr, enabled ? enabled_bulk_data : disabled_bulk_data);

    if (enabled)
    { // AMP on
      M5.In_I2C.bitOn(pi4io1_i2c_addr, 0x05, 0b00000010, 400000);
    }
    else
    { // AMP off
      M5.In_I2C.bitOff(pi4io1_i2c_addr, 0x05, 0b00000010, 400000);
    }
#endif
    return true;
  }

  bool M5Unified::_speaker_enabled_cb_hat_spk(void* args, bool enabled)
  {
    (void)args;
    (void)enabled;
#if defined (CONFIG_IDF_TARGET_ESP32)
    auto self = (M5Unified*)args;
    gpio_num_t pin_en = self->_board == board_t::board_M5StackCoreInk ? GPIO_NUM_25 : GPIO_NUM_0;
    if (enabled)
    {
      m5gfx::pinMode(pin_en, m5gfx::pin_mode_t::output);
      m5gfx::gpio_hi(pin_en);
    }
    else
    { m5gfx::gpio_lo(pin_en); }
#endif
    return true;
  }

  bool M5Unified::_speaker_enabled_cb_atomic_echo(void* args, bool enabled)
  {
    (void)args;
    (void)enabled;
    static constexpr const uint8_t enabled_bulk_data[] = {
      2, 0x00, 0x80,  // 0x00 RESET/  CSM POWER ON
      2, 0x01, 0xB5,  // 0x01 CLOCK_MANAGER/ MCLK=BCLK
      2, 0x02, 0x18,  // 0x02 CLOCK_MANAGER/ MULT_PRE=3
      2, 0x0D, 0x01,  // 0x0D SYSTEM/ Power up analog circuitry
      2, 0x12, 0x00,  // 0x12 SYSTEM/ power-up DAC - NOT default
      2, 0x13, 0x10,  // 0x13 SYSTEM/ Enable output to HP drive - NOT default
      2, 0x32, 0xFF,  // 0x32 DAC/ DAC volume (full volume)
      2, 0x37, 0x08,  // 0x37 DAC/ Bypass DAC equalizer - NOT default
      0
    };
    static constexpr const uint8_t disabled_bulk_data[] = {
      0
    };

    static constexpr const uint8_t enabled_pi4ioe_bulk_data[] = {
      2, 0x03, 0xFF,  // PI4IOE direction:OUTPUT
      2, 0x05, 0xFF,  // PI4IOE output HIGH
      2, 0x07, 0x00,  // PI4IOE set push-pull
      2, 0x0B, 0x00,  // Disable pull (up and down)
      0
    };
    static constexpr const uint8_t disabled_pi4ioe_bulk_data[] = {
      2, 0x05, 0x00,
      0
    };

#if defined (CONFIG_IDF_TARGET_ESP32S3)
    m5gfx::i2c::i2c_temporary_switcher_t backup_i2c_setting(1, GPIO_NUM_38, GPIO_NUM_39);
#endif
    in_i2c_bulk_write(es8311_i2c_addr0, enabled ? enabled_bulk_data : disabled_bulk_data);
    in_i2c_bulk_write(pi4io1_i2c_addr, enabled ? enabled_pi4ioe_bulk_data : disabled_pi4ioe_bulk_data);
#if defined (CONFIG_IDF_TARGET_ESP32S3)
    backup_i2c_setting.restore();
#endif
    return true;
  }

  bool M5Unified::_microphone_enabled_cb_stickc(void* args, bool enabled)
  {
    (void)args;
    (void)enabled;
#if defined (CONFIG_IDF_TARGET_ESP32)
    auto self = (M5Unified*)args;
    self->Power.Axp192.setLDO0(enabled ? 2800 : 0);
#endif
    return true;
  }

  bool M5Unified::_microphone_enabled_cb_cores3(void* args, bool enabled)
  {
    (void)args;
    (void)enabled;
#if defined (CONFIG_IDF_TARGET_ESP32S3)
    auto self = (M5Unified*)args;
    auto cfg = self->Mic.config();
    if (cfg.pin_bck == GPIO_NUM_34)
    {
      es7210_write_reg(0x00, 0xFF); // RESET_CTL
      struct __attribute__((packed)) reg_data_t
      {
        uint8_t reg;
        uint8_t value;
      };
      if (enabled)
      {
        static constexpr reg_data_t data[] =
        {
          { 0x00, 0x41 }, // RESET_CTL
          { 0x01, 0x1f }, // CLK_ON_OFF
          { 0x06, 0x00 }, // DIGITAL_PDN
          { 0x07, 0x20 }, // ADC_OSR
          { 0x08, 0x10 }, // MODE_CFG
          { 0x09, 0x30 }, // TCT0_CHPINI
          { 0x0A, 0x30 }, // TCT1_CHPINI
          { 0x20, 0x0a }, // ADC34_HPF2
          { 0x21, 0x2a }, // ADC34_HPF1
          { 0x22, 0x0a }, // ADC12_HPF2
          { 0x23, 0x2a }, // ADC12_HPF1
          { 0x02, 0xC1 },
          { 0x04, 0x01 },
          { 0x05, 0x00 },
          { 0x11, 0x60 },
          { 0x40, 0x42 }, // ANALOG_SYS
          { 0x41, 0x70 }, // MICBIAS12
          { 0x42, 0x70 }, // MICBIAS34
          { 0x43, 0x1B }, // MIC1_GAIN
          { 0x44, 0x1B }, // MIC2_GAIN
          { 0x45, 0x00 }, // MIC3_GAIN
          { 0x46, 0x00 }, // MIC4_GAIN
          { 0x47, 0x00 }, // MIC1_LP
          { 0x48, 0x00 }, // MIC2_LP
          { 0x49, 0x00 }, // MIC3_LP
          { 0x4A, 0x00 }, // MIC4_LP
          { 0x4B, 0x00 }, // MIC12_PDN
          { 0x4C, 0xFF }, // MIC34_PDN
          { 0x01, 0x14 }, // CLK_ON_OFF
        };
        for (auto& d: data)
        {
          es7210_write_reg(d.reg, d.value);
        }
      }
    }
#endif
    return true;
  }

  bool M5Unified::_microphone_enabled_cb_atomic_echo(void* args, bool enabled)
  {
    (void)args;
    (void)enabled;
    static constexpr const uint8_t enabled_bulk_data[] = {
      2, 0x00, 0x80,  // 0x00 RESET/  CSM POWER ON
      2, 0x01, 0xBA,  // 0x01 CLOCK_MANAGER/ MCLK=BCLK
      2, 0x02, 0x18,  // 0x02 CLOCK_MANAGER/ MULT_PRE=3
      2, 0x0D, 0x01,  // 0x0D SYSTEM/ Power up analog circuitry
      2, 0x0E, 0x02,  // 0x0E SYSTEM/ : Enable analog PGA, enable ADC modulator
      2, 0x14, 0x10,  // ES8311_ADC_REG14 : select Mic1p-Mic1n / PGA GAIN (minimum)
      2, 0x17, 0xFF,  // ES8311_ADC_REG17 : ADC_VOLUME (MAXGAIN) // (0xBF == ± 0 dB )
      2, 0x1C, 0x6A,  // ES8311_ADC_REG1C : ADC Equalizer bypass, cancel DC offset in digital domain
      0
    };
    static constexpr const uint8_t disabled_bulk_data[] = {
      2, 0x0D, 0xFC,  // 0x0D SYSTEM/ Power down analog circuitry
      2, 0x0E, 0x6A,  // 0x0E SYSTEM
      2, 0x00, 0x00,  // 0x00 RESET/  CSM POWER DOWN
      0
    };
#if defined (CONFIG_IDF_TARGET_ESP32S3)
    m5gfx::i2c::i2c_temporary_switcher_t backup_i2c_setting(1, GPIO_NUM_38, GPIO_NUM_39);
#endif
    in_i2c_bulk_write(es8311_i2c_addr0, enabled ? enabled_bulk_data : disabled_bulk_data);
#if defined (CONFIG_IDF_TARGET_ESP32S3)
    backup_i2c_setting.restore();
#endif

    return true;
  }

  bool M5Unified::_microphone_enabled_cb_tab5(void* args, bool enabled)
  {
    (void)args;
    (void)enabled;
#if defined (CONFIG_IDF_TARGET_ESP32P4)
    auto self = (M5Unified*)args;
    auto cfg = self->Mic.config();
    if (cfg.pin_data_in != GPIO_NUM_28) { return false; }

    M5.In_I2C.writeRegister8(es7210_i2c_addr, 0x00, 0xFF, 400000);
    if (enabled)
    {
      static constexpr uint8_t data[] =
      {
        2, 0x00, 0x41, // RESET_CTL
        2, 0x01, 0x1f, // CLK_ON_OFF
        2, 0x06, 0x00, // DIGITAL_PDN
        2, 0x07, 0x20, // ADC_OSR
        2, 0x08, 0x10, // MODE_CFG
        2, 0x09, 0x30, // TCT0_CHPINI
        2, 0x0A, 0x30, // TCT1_CHPINI
        2, 0x20, 0x0a, // ADC34_HPF2
        2, 0x21, 0x2a, // ADC34_HPF1
        2, 0x22, 0x0a, // ADC12_HPF2
        2, 0x23, 0x2a, // ADC12_HPF1
        2, 0x02, 0xC1,
        2, 0x04, 0x01,
        2, 0x05, 0x00,
        2, 0x11, 0x60,
        2, 0x40, 0x42, // ANALOG_SYS
        2, 0x41, 0x70, // MICBIAS12
        2, 0x42, 0x70, // MICBIAS34
        2, 0x43, 0x1B, // MIC1_GAIN
        2, 0x44, 0x1B, // MIC2_GAIN
        2, 0x45, 0x00, // MIC3_GAIN
        2, 0x46, 0x00, // MIC4_GAIN
        2, 0x47, 0x00, // MIC1_LP
        2, 0x48, 0x00, // MIC2_LP
        2, 0x49, 0x00, // MIC3_LP
        2, 0x4A, 0x00, // MIC4_LP
        2, 0x4B, 0x00, // MIC12_PDN
        2, 0x4C, 0xFF, // MIC34_PDN
        2, 0x01, 0x14, // CLK_ON_OFF
        0,
      };
      in_i2c_bulk_write(es7210_i2c_addr, data);
    }
#endif
    return true;
  }



#if defined (M5UNIFIED_PC_BUILD)
#elif !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
  static constexpr gpio_num_t TFCARD_CS_PIN          = GPIO_NUM_4;
  static constexpr gpio_num_t CoreInk_BUTTON_EXT_PIN = GPIO_NUM_5;
  static constexpr gpio_num_t CoreInk_BUTTON_PWR_PIN = GPIO_NUM_27;
#endif

  board_t M5Unified::_check_boardtype(board_t board)
  {
#if defined (M5UNIFIED_PC_BUILD)
#elif !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
    if (board == board_t::board_unknown)
    {
      switch (m5gfx::get_pkg_ver())
      {
      case EFUSE_RD_CHIP_VER_PKG_ESP32D0WDQ6:
        board = board_t::board_M5TimerCam;
        break;

      case EFUSE_RD_CHIP_VER_PKG_ESP32PICOD4:
        {
          m5gfx::gpio::pin_backup_t pin_backup[] = { GPIO_NUM_2, GPIO_NUM_13, GPIO_NUM_19, GPIO_NUM_22, GPIO_NUM_27, GPIO_NUM_33, GPIO_NUM_34 };
          m5gfx::pinMode(GPIO_NUM_34, m5gfx::pin_mode_t::input);
          m5gfx::pinMode(GPIO_NUM_2, m5gfx::pin_mode_t::input_pullup);
          board = board_t::board_M5StampPico;
          if (m5gfx::gpio_in(GPIO_NUM_2)) // Branches other than StampPico ( StampPico G2 is always LOW )
          {
            board = board_t::board_M5AtomU;
            if (m5gfx::gpio_in(GPIO_NUM_34)) { // Branches other than AtomU ( AtomU G34 is always LOW )
              board = board_t::board_M5AtomMatrix;
#if SOC_TOUCH_SENSOR_SUPPORTED
/* G27(RGBLED)に対してタッチセンサを用い、容量の差に基づいて Matrix の識別を行う。
  G27に対してタッチセンサを使用すると、得られる値は Lite/ECHOの方が大きく、Matrixの方が小さい。
  なおタッチセンサの値には個体差があるため、判定の基準として絶対値ではなく G13(NC)のタッチセンサ値を比較に用いる。
*/
              uint16_t g13, g27;
              touch_pad_init();
              touch_pad_config(TOUCH_PAD_NUM4, TOUCH_PAD_THRESHOLD_MAX);  // TOUCH_PAD_NUM4 == GPIO13
              touch_pad_config(TOUCH_PAD_NUM7, TOUCH_PAD_THRESHOLD_MAX);  // TOUCH_PAD_NUM7 == GPIO27
              touch_pad_read(TOUCH_PAD_NUM4, &g13);
              touch_pad_read(TOUCH_PAD_NUM7, &g27);
              touch_pad_deinit();
              int diff = (g27 * 3 - g13);
              // M5_LOGV("G13 = %d / G27 = %d / diff = %d", g13, g27, diff);

              // Branches other than AtomMatrix
              if (diff >= 0)
#else
/*
  タッチセンサAPIが使えない場合の処理 (ESP-IDFのバージョンに依る)
  GPIOの立上り速度の差を用いて LiteとMatrix の識別を行う。
  (Matrixの方がinput_pullupでHIGHになるまでの時間が長いため、この性質を利用して判定する)
*/
              portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
              uint32_t g27 = 0;
              // 8回読み取って立上り速度の差を見る
              for (int i = 0; i < 8; ++i)
              {
                lgfx::pinMode(GPIO_NUM_27, lgfx::pin_mode_t::input_pulldown);
                delay(1);
                taskENTER_CRITICAL(&mux);
                lgfx::pinMode(GPIO_NUM_27, lgfx::pin_mode_t::input_pullup);
                g27 += lgfx::gpio_in(GPIO_NUM_27);
                taskEXIT_CRITICAL(&mux);
              }

              // Branches other than AtomMatrix ( AtomMatrix G27 is delayed from becoming HIGH )
              if (g27 > 4)
#endif
              {
                auto result = m5gfx::gpio::command(
                  (const uint8_t[]) {
                  m5gfx::gpio::command_mode_input_pulldown, GPIO_NUM_22,
                  m5gfx::gpio::command_mode_input_pulldown, GPIO_NUM_19,
                  m5gfx::gpio::command_mode_input_pulldown, GPIO_NUM_33,
                  m5gfx::gpio::command_mode_input_pullup  , GPIO_NUM_33,
                  m5gfx::gpio::command_mode_input_pullup  , GPIO_NUM_19,
                  m5gfx::gpio::command_mode_input_pullup  , GPIO_NUM_22,
                  m5gfx::gpio::command_read               , GPIO_NUM_33,
                  m5gfx::gpio::command_read               , GPIO_NUM_19,
                  m5gfx::gpio::command_read               , GPIO_NUM_22,
                  m5gfx::gpio::command_mode_input         , GPIO_NUM_33,
                  m5gfx::gpio::command_mode_input         , GPIO_NUM_19,
                  m5gfx::gpio::command_mode_input         , GPIO_NUM_22,
                  m5gfx::gpio::command_delay              , 1,
                  m5gfx::gpio::command_read               , GPIO_NUM_33,
                  m5gfx::gpio::command_read               , GPIO_NUM_19,
                  m5gfx::gpio::command_read               , GPIO_NUM_22,
                  }
                );
                // G19 G22 G33 = ECHOのI2Sスピーカ用ピン。プルアップを無効化するとすぐにLOWになるため、この性質を利用して判定する。
                // なお当該ピンに何かを外付けしている場合は判定に失敗する可能性がある。
                board = board_t::board_M5AtomLite;
                if ((result) == 0b111000)
                { // Branches for AtomECHO
                  board = board_t::board_M5AtomEcho;
                }
              }
            }
          }
          for (auto &backup : pin_backup) {
            backup.restore();
          }
        }
        break;

      case 6: // EFUSE_RD_CHIP_VER_PKG_ESP32PICOV3_02: // ATOM PSRAM
        board = board_t::board_M5AtomPsram;
        break;

      default:

#if defined ( ARDUINO_M5STACK_CORE_ESP32 ) || defined ( ARDUINO_M5STACK_FIRE ) || defined ( ARDUINO_M5Stack_Core_ESP32 )

        board = board_t::board_M5Stack;

#elif defined ( ARDUINO_M5STACK_CORE2 ) || defined ( ARDUINO_M5STACK_Core2 )

        board = board_t::board_M5StackCore2;

#elif defined ( ARDUINO_M5STICK_C ) || defined ( ARDUINO_M5Stick_C )

        board = board_t::board_M5StickC;

#elif defined ( ARDUINO_M5STICK_C_PLUS ) || defined ( ARDUINO_M5Stick_C_Plus )

        board = board_t::board_M5StickCPlus;

#elif defined ( ARDUINO_M5STACK_COREINK ) || defined ( ARDUINO_M5Stack_CoreInk )

        board = board_t::board_M5StackCoreInk;

#elif defined ( ARDUINO_M5STACK_PAPER ) || defined ( ARDUINO_M5STACK_Paper )

        board = board_t::board_M5Paper;

#elif defined ( ARDUINO_M5STACK_TOUGH )

        board = board_t::board_M5Tough;

#elif defined ( ARDUINO_M5STACK_ATOM ) || defined ( ARDUINO_M5Stack_ATOM )

        board = board_t::board_M5AtomLite;

#elif defined ( ARDUINO_M5STACK_TIMER_CAM ) || defined ( ARDUINO_M5Stack_Timer_CAM )

        board = board_t::board_M5TimerCam;

#endif
        break;
      }
    }

#elif defined (CONFIG_IDF_TARGET_ESP32S3)

    switch (m5gfx::get_pkg_ver())
    {
    default:
    case 0: // EFUSE_PKG_VERSION_ESP32S3:     // QFN56
      if (board == board_t::board_unknown)
      { /// StampS3 or AtomS3Lite,S3U ?
        ///   After setting GPIO38 to INPUT PULL-UP, change to INPUT and read it.
        ///   In the case of STAMPS3: Returns 0. Charge is sucked by SGM2578.
        ///   In the case of ATOMS3Lite/S3U : Returns 1. Charge remains. ( Since it is not connected to anywhere. )
        ///
        /// AtomS3Lite or AtomS3U ?
        ///   After setting GPIO4 to INPUT PULL-UP, read it.
        ///   In the case of ATOMS3Lite : Returns 0. Charge is sucked by InfraRed.
        ///   In the case of ATOMS3U    : Returns 1. Charge remains. ( Since it is not connected to anywhere. )

        m5gfx::gpio::pin_backup_t pin_backup[] = { GPIO_NUM_4, GPIO_NUM_8, GPIO_NUM_10, GPIO_NUM_12, GPIO_NUM_38 };
        auto result = m5gfx::gpio::command(
          (const uint8_t[]) {
          m5gfx::gpio::command_mode_input_pulldown, GPIO_NUM_4,
          m5gfx::gpio::command_mode_input_pulldown, GPIO_NUM_12,
          m5gfx::gpio::command_mode_input_pulldown, GPIO_NUM_38,
          m5gfx::gpio::command_mode_input_pulldown, GPIO_NUM_8,
          m5gfx::gpio::command_mode_input_pulldown, GPIO_NUM_10,
          m5gfx::gpio::command_mode_input_pullup  , GPIO_NUM_4,
          m5gfx::gpio::command_mode_input_pullup  , GPIO_NUM_12,
          m5gfx::gpio::command_mode_input_pullup  , GPIO_NUM_38,
          m5gfx::gpio::command_read               , GPIO_NUM_8,
          m5gfx::gpio::command_read               , GPIO_NUM_10,
          m5gfx::gpio::command_read               , GPIO_NUM_4,
          m5gfx::gpio::command_read               , GPIO_NUM_12,
          m5gfx::gpio::command_read               , GPIO_NUM_38,
          m5gfx::gpio::command_mode_input         , GPIO_NUM_38,
          m5gfx::gpio::command_delay              , 1,
          m5gfx::gpio::command_read               , GPIO_NUM_38,
          m5gfx::gpio::command_end
          }
        );
        /// result には、command_read で得たGPIOの状態が1bitずつ4回分入っている。
        board = ((const board_t[])
          { //                                                      ↓StampS3 pattern↓
            board_t::board_unknown,     board_t::board_unknown,     board_t::board_M5StampS3, board_t::board_unknown,      // ← unknown
            board_t::board_M5AtomS3Lite,board_t::board_M5AtomS3Lite,board_t::board_unknown  , board_t::board_M5AtomS3Lite, // ← AtomS3Lite pattern
            board_t::board_M5AtomS3U,   board_t::board_M5AtomS3U,   board_t::board_M5StampS3, board_t::board_M5AtomS3U,    // ← AtomS3U pattern
            board_t::board_unknown,     board_t::board_unknown,     board_t::board_M5StampS3, board_t::board_unknown,      // ← unknown
          })[result&15];
        if ((result & 3) == 2) { // StampS3 pattern
          if ((result >> 3) == 0b110) {
            board = board_t::board_M5Capsule;
            // 自動検出の際。PortAに余分な波形が出ているので、一度 I2C STOPコンディションを出しておく。
            // ※ これをしないと正しく動作しないデバイスが存在した。UnitHEART MAX30100
            m5gfx::gpio::command(
              (const uint8_t[]) {
              m5gfx::gpio::command_mode_output, GPIO_NUM_15,
              m5gfx::gpio::command_write_low  , GPIO_NUM_15,
              m5gfx::gpio::command_mode_output, GPIO_NUM_13,
              m5gfx::gpio::command_write_low  , GPIO_NUM_13,
              m5gfx::gpio::command_write_high , GPIO_NUM_15,
              m5gfx::gpio::command_write_high , GPIO_NUM_13,
              m5gfx::gpio::command_end
              }
            );
          }
        }
        for (auto &backup : pin_backup) {
          backup.restore();
        }
      }
      break;

    case 1: // EFUSE_PKG_VERSION_ESP32S3PICO: // LGA56
      if (board == board_t::board_unknown)
      { /// AtomS3RCam or AtomS3RExt ?
      // Cam    = GC0308 = I2C 7bit addr = 0x21
      // CamM12 = OV3660 = I2C 7bit addr = 0x3C
        board = board_t::board_M5AtomS3RExt;
        m5gfx::gpio_lo(GPIO_NUM_18);
        m5gfx::pinMode(GPIO_NUM_18, m5gfx::pin_mode_t::output);
        m5gfx::gpio::pin_backup_t pin_backup[] = { GPIO_NUM_9, GPIO_NUM_12, GPIO_NUM_21 };
        { // G9=SCL, G12=SDA, G21=XCLK
          m5gfx::gpio::command(
            (const uint8_t[]) {
            m5gfx::gpio::command_write_low, GPIO_NUM_9,
            m5gfx::gpio::command_mode_output, GPIO_NUM_9,  // SCL
            m5gfx::gpio::command_write_low, GPIO_NUM_12,
            m5gfx::gpio::command_mode_output, GPIO_NUM_12, // SDA
            m5gfx::gpio::command_mode_output, GPIO_NUM_21, // XCL
            m5gfx::gpio::command_write_high, GPIO_NUM_9,
            m5gfx::gpio::command_write_high, GPIO_NUM_21,
            m5gfx::gpio::command_write_high, GPIO_NUM_12,
          });
          auto lo_reg = m5gfx::get_gpio_lo_reg(GPIO_NUM_21);
          auto hi_reg = m5gfx::get_gpio_hi_reg(GPIO_NUM_21);

          // prepare camera module (need XCLK signal)
          for (int xclk = 32768 * 54; xclk != 0; --xclk)
          {
            *lo_reg = 1 << GPIO_NUM_21;
            *hi_reg = 1 << GPIO_NUM_21;
          }
          uint32_t result = 0;
          for (uint8_t i2caddr: (const uint8_t[]){ 0x3C << 1, 0x21 << 1 }) {
            for (int xclk = 32768 * 2; xclk != 0; --xclk) {
              *lo_reg = 1 << GPIO_NUM_21;
              *hi_reg = 1 << GPIO_NUM_21;
            }
            bool nack = true;
            // The camera module is identified using I2C communication via GPIO self-operation.
            *lo_reg = 1 << GPIO_NUM_12;  // SDA LOW = START
            for (int cycle = 0; cycle < 20; ++cycle) {
              for (int j = 0; j < 2; ++j) {
                for (int xclk = 8; xclk != 0; --xclk) {
                  *lo_reg = 1 << GPIO_NUM_21;
                  *hi_reg = 1 << GPIO_NUM_21;
                }
                *((cycle & 1) ? hi_reg : lo_reg) = 1 << GPIO_NUM_9; // SCL
              }
              if (cycle & 1) {
                if (cycle == 17) {
                  nack = m5gfx::gpio_in(GPIO_NUM_12);
                }
              } else {
                *((i2caddr & 0x80) ? hi_reg : lo_reg) = 1 << GPIO_NUM_12; // SDA
                i2caddr <<= 1;
                if (cycle >= 16) {
                  m5gfx::pinMode(GPIO_NUM_12, (cycle == 16) ? m5gfx::pin_mode_t::input : m5gfx::pin_mode_t::output);
                }
              }
            }
            *hi_reg = 1 << GPIO_NUM_12;  // SDA HIGH = STOP
            result = result << 1 | nack;
          }
          // printf("CAM TEST  RESULT: %08x \r\n", (int)result);
          if (result == 1 || result == 2) {
          // result == 1 : OV3660
          // result == 2 : GC0308
            board = board_t::board_M5AtomS3RCam;
          }
        }
        for (auto &backup : pin_backup) {
          backup.restore();
        }
      }
    }


#elif defined (CONFIG_IDF_TARGET_ESP32C3)
    if (board == board_t::board_unknown)
    { // StampC3 or StampC3U ?
      uint32_t tmp = *((volatile uint32_t *)(IO_MUX_GPIO20_REG));
      m5gfx::pinMode(GPIO_NUM_20, m5gfx::pin_mode_t::input_pulldown);
      // StampC3 has a strong external pull-up on GPIO20, which is HIGH even when input_pulldown is set.
      // Therefore, if it is LOW, it is not StampC3 and can be assumed to be StampC3U.
      // However, even if it goes HIGH, something may be connected to GPIO20 by StampC3U, so it is treated as unknown.
      // The StampC3U determination uses the fallback_board setting.
      if (m5gfx::gpio_in(GPIO_NUM_20) == false)
      {
        board = board_t::board_M5StampC3U;
      }
      *((volatile uint32_t *)(IO_MUX_GPIO20_REG)) = tmp;
    }

#elif defined (CONFIG_IDF_TARGET_ESP32C6)
    if (board == board_t::board_unknown)
    { // NanoC6
      board = board_t::board_M5NanoC6;
    }

#elif defined (CONFIG_IDF_TARGET_ESP32P4)
    if (board == board_t::board_unknown)
    {
      board = board_t::board_M5Tab5;
    }

#endif

    return board;
  }

  void M5Unified::_setup_i2c(board_t board)
  {
#if defined (M5UNIFIED_PC_BUILD)
    (void)board;
#else

    gpio_num_t in_scl = (gpio_num_t)getPin(pin_name_t::in_i2c_scl);
    gpio_num_t in_sda = (gpio_num_t)getPin(pin_name_t::in_i2c_sda);
    gpio_num_t ex_scl = (gpio_num_t)getPin(pin_name_t::ex_i2c_scl);
    gpio_num_t ex_sda = (gpio_num_t)getPin(pin_name_t::ex_i2c_sda);

    i2c_port_t ex_port = I2C_NUM_0;
#if SOC_I2C_NUM == 1 || defined (CONFIG_IDF_TARGET_ESP32C6)
    i2c_port_t in_port = I2C_NUM_0;
#else
    i2c_port_t in_port = I2C_NUM_1;
    if (in_scl == ex_scl && in_sda == ex_sda) {
      in_port = ex_port;
    }
#endif
    if ((int)in_scl >= 0)
    {
      In_I2C.begin(in_port, in_sda, in_scl);
    }
    else
    {
      In_I2C.setPort(I2C_NUM_MAX, in_sda, in_scl);
    }

    if ((int)ex_scl >= 0)
    {
      Ex_I2C.setPort(ex_port, ex_sda, ex_scl);
    }
#endif

  }

  void M5Unified::_begin(const config_t& cfg)
  {
    /// setup power management ic
    Power.begin();
    Power.setExtOutput(cfg.output_power);
    if (cfg.led_brightness)
    {
      M5.Power.setLed(cfg.led_brightness);
    }
    auto pmic_type = Power.getType();
    if (pmic_type == Power_Class::pmic_t::pmic_axp2101
     || pmic_type == Power_Class::pmic_t::pmic_axp192)
    {
      use_pmic_button = cfg.pmic_button;
      /// Slightly lengthen the acceptance time of the AXP192 power button multiclick.
      BtnPWR.setHoldThresh(BtnPWR.getHoldThresh() * 1.2);
    }

    if (cfg.clear_display)
    {
      Display.clear();
    }

#if defined (M5UNIFIED_PC_BUILD)
#elif !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
    switch (_board)
    {
    case board_t::board_M5Stack:
      // Countermeasure to the problem that GPIO15 affects WiFi sensitivity when M5GO bottom is connected.
      m5gfx::pinMode(GPIO_NUM_15, m5gfx::pin_mode_t::output);
      m5gfx::gpio_lo(GPIO_NUM_15);

      // M5Stack Core v2.6 has a problem that SPI communication speed cannot be increased.
      // This problem can be solved by increasing the GPIO drive current.
      // ※  This allows SunDisk SD cards to communicate at 20 MHz. (without M5GO bottom.)
      //     This allows communication with ModuleDisplay at 80 MHz.
      for (auto gpio: (const gpio_num_t[]){ GPIO_NUM_18, GPIO_NUM_19, GPIO_NUM_23 })
      {
        uint32_t tmp = *(volatile uint32_t*)(GPIO_PIN_MUX_REG[gpio]);
        *(volatile uint32_t*)(GPIO_PIN_MUX_REG[gpio]) = tmp | FUN_DRV_M; // gpio drive current set to 40mA.
        gpio_pulldown_dis(gpio); // disable pulldown.
        gpio_pullup_en(gpio);    // enable pullup.
      }
      break;

    case board_t::board_M5StickC:
    case board_t::board_M5StickCPlus:
    case board_t::board_M5AtomLite:
    case board_t::board_M5AtomMatrix:
    case board_t::board_M5AtomEcho:
    case board_t::board_M5AtomU:
      // Countermeasure to the problem that CH552 applies 4v to GPIO0, thus reducing WiFi sensitivity.
      // Setting output_high adds a bias of 3.3v and suppresses overvoltage.
      m5gfx::pinMode(GPIO_NUM_0, m5gfx::pin_mode_t::output);
      m5gfx::gpio_hi(GPIO_NUM_0);
      break;

    default:
      break;
    }
#endif

    switch (_board) /// setup Hardware Buttons
    {
#if defined (M5UNIFIED_PC_BUILD)
#elif !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
    case board_t::board_M5StackCoreInk:
      m5gfx::pinMode(CoreInk_BUTTON_EXT_PIN, m5gfx::pin_mode_t::input); // TopButton
      m5gfx::pinMode(CoreInk_BUTTON_PWR_PIN, m5gfx::pin_mode_t::input); // PowerButton
      NON_BREAK; /// don't break;

    case board_t::board_M5Paper:
    case board_t::board_M5Station:
    case board_t::board_M5Stack:
      m5gfx::pinMode(GPIO_NUM_38, m5gfx::pin_mode_t::input);
      NON_BREAK; /// don't break;

    case board_t::board_M5StickC:
    case board_t::board_M5StickCPlus:
      m5gfx::pinMode(GPIO_NUM_37, m5gfx::pin_mode_t::input);
      NON_BREAK; /// don't break;

    case board_t::board_M5AtomLite:
    case board_t::board_M5AtomMatrix:
    case board_t::board_M5AtomEcho:
    case board_t::board_M5AtomPsram:
    case board_t::board_M5AtomU:
    case board_t::board_M5StampPico:
      m5gfx::pinMode(GPIO_NUM_39, m5gfx::pin_mode_t::input);
      NON_BREAK; /// don't break;

    case board_t::board_M5StackCore2:
    case board_t::board_M5Tough:
 /// for GPIO 36,39 Chattering prevention.
      adc_power_acquire();
      break;

    case board_t::board_M5StickCPlus2:
      m5gfx::pinMode(GPIO_NUM_35, m5gfx::pin_mode_t::input);
      m5gfx::pinMode(GPIO_NUM_37, m5gfx::pin_mode_t::input);
      m5gfx::pinMode(GPIO_NUM_39, m5gfx::pin_mode_t::input);
      break;

#elif defined (CONFIG_IDF_TARGET_ESP32C3)

    case board_t::board_M5StampC3:
      m5gfx::pinMode(GPIO_NUM_3, m5gfx::pin_mode_t::input_pullup);
      break;

    case board_t::board_M5StampC3U:
      m5gfx::pinMode(GPIO_NUM_9, m5gfx::pin_mode_t::input_pullup);
      break;

#elif defined (CONFIG_IDF_TARGET_ESP32C6)

    case board_t::board_M5NanoC6:
      m5gfx::pinMode(GPIO_NUM_9, m5gfx::pin_mode_t::input_pullup);
      break;

#elif defined (CONFIG_IDF_TARGET_ESP32S3)
    case board_t::board_M5AtomS3:
    case board_t::board_M5AtomS3Lite:
    case board_t::board_M5AtomS3U:
    case board_t::board_M5AtomS3R:
      m5gfx::pinMode(GPIO_NUM_41, m5gfx::pin_mode_t::input);
      break;

    case board_t::board_M5AirQ:
      m5gfx::pinMode(GPIO_NUM_0, m5gfx::pin_mode_t::input);
      m5gfx::pinMode(GPIO_NUM_8, m5gfx::pin_mode_t::input);
      break;

    case board_t::board_M5VAMeter:
      m5gfx::pinMode(GPIO_NUM_0, m5gfx::pin_mode_t::input);
      m5gfx::pinMode(GPIO_NUM_2, m5gfx::pin_mode_t::input);
      break;

    case board_t::board_M5StampS3:
    case board_t::board_M5Cardputer:
      m5gfx::pinMode(GPIO_NUM_0, m5gfx::pin_mode_t::input);
      break;

    case board_t::board_M5Capsule:
    case board_t::board_M5Dial:
    case board_t::board_M5DinMeter:
      m5gfx::pinMode(GPIO_NUM_42, m5gfx::pin_mode_t::input);
      break;

    case board_t::board_M5StampPLC:
      _io_expander_a_init();

      // lcd backlight
      _io_expander_a->setDirection(7, true);
      _io_expander_a->setPullMode(7, false);
      _io_expander_a->setHighImpedance(7, false);

      for (int i = 0; i < 3; ++i) {
        // button a~c
        _io_expander_a->setDirection(i, false);
        _io_expander_a->setPullMode(i, true);
        _io_expander_a->setHighImpedance(i, false);
      }

      delay(100);
      break;

#endif

    default:
      break;
    }

#if defined ( ARDUINO )

    if (cfg.serial_baudrate)
    { // Wait with delay to prevent startup log output from disappearing.
      delay(16);
      Serial.begin(cfg.serial_baudrate);
    }

#endif
  }

  void M5Unified::_begin_spk(config_t& cfg)
  {
    bool(*mic_enable_cb)(void*, bool) = nullptr;
    auto mic_cfg = Mic.config();

    bool(*spk_enable_cb)(void*, bool) = nullptr;
    auto spk_cfg = Speaker.config();

    if (cfg.internal_mic)
    {
      mic_cfg.over_sampling = 1;
      mic_cfg.i2s_port = I2S_NUM_0;
      switch (_board)
      {
#if defined (M5UNIFIED_PC_BUILD)
#elif defined (CONFIG_IDF_TARGET_ESP32P4)
      case board_t::board_M5Tab5:
        if (cfg.internal_mic)
        {
          mic_cfg.pin_mck = GPIO_NUM_30;
          mic_cfg.pin_bck = GPIO_NUM_27;
          mic_cfg.pin_ws = GPIO_NUM_29;
          mic_cfg.pin_data_in = GPIO_NUM_28;
          // mic_cfg.pin_data_out = GPIO_NUM_26;
          mic_cfg.magnification = 2;
          mic_cfg.input_channel = input_channel_t::input_stereo;
          mic_cfg.i2s_port = I2S_NUM_0;
          mic_enable_cb = _microphone_enabled_cb_tab5;
        }
        break;

#elif defined (CONFIG_IDF_TARGET_ESP32S3)
      case board_t::board_M5StackCoreS3:
      case board_t::board_M5StackCoreS3SE:
        if (cfg.internal_mic)
        {
          mic_cfg.magnification = 2;
          mic_cfg.over_sampling = 1;
          mic_cfg.pin_mck = GPIO_NUM_0;
          mic_cfg.pin_bck = GPIO_NUM_34;
          mic_cfg.pin_ws = GPIO_NUM_33;
          mic_cfg.pin_data_in = GPIO_NUM_14;
          mic_cfg.i2s_port = I2S_NUM_1;
          mic_cfg.input_channel = input_channel_t::input_stereo;
          mic_enable_cb = _microphone_enabled_cb_cores3;
        }
        break;

      case board_t::board_M5AtomS3U:
        if (cfg.internal_mic)
        {
          mic_cfg.pin_data_in = GPIO_NUM_38;
          mic_cfg.pin_ws = GPIO_NUM_39;
        }
        break;

      case board_t::board_M5Cardputer:
        if (cfg.internal_mic)
        {
          mic_cfg.pin_data_in = GPIO_NUM_46;
          mic_cfg.pin_ws = GPIO_NUM_43;
        }
        break;

      case board_t::board_M5Capsule:
        if (cfg.internal_mic)
        {
          mic_cfg.pin_data_in = GPIO_NUM_41;
          mic_cfg.pin_ws = GPIO_NUM_40;
        }
        break;

#elif !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
      case board_t::board_M5Stack:
        if (cfg.internal_mic)
        {
          mic_cfg.pin_data_in = GPIO_NUM_34;  // M5GO bottom MIC
          mic_cfg.i2s_port = I2S_NUM_0;
          mic_cfg.use_adc = true;    // use ADC analog input
          mic_cfg.over_sampling = 4;
        }
        break;

      case board_t::board_M5StickC:
      case board_t::board_M5StickCPlus:
        if (cfg.internal_mic)
        { /// builtin PDM mic
          mic_cfg.pin_data_in = GPIO_NUM_34;
          mic_cfg.pin_ws = GPIO_NUM_0;
          mic_enable_cb = _microphone_enabled_cb_stickc;
        }
        break;

      case board_t::board_M5StickCPlus2:
      case board_t::board_M5Tough:
      case board_t::board_M5StackCore2:
        if (cfg.internal_mic)
        { /// builtin PDM mic
          mic_cfg.pin_data_in = GPIO_NUM_34;
          mic_cfg.pin_ws = GPIO_NUM_0;
        }
        break;

      case board_t::board_M5AtomU:
        { /// ATOM U builtin PDM mic
          mic_cfg.pin_data_in = GPIO_NUM_19;
          mic_cfg.pin_ws = GPIO_NUM_5;
        }
        break;

      case board_t::board_M5AtomEcho:
        { /// ATOM ECHO builtin PDM mic
          mic_cfg.pin_data_in = GPIO_NUM_23;
          mic_cfg.pin_ws = GPIO_NUM_33;
        }
        break;
#endif
      default:
        break;
      }
    }

    if (cfg.external_spk_detail.enabled && cfg.external_speaker_value == 0) {
      cfg.external_speaker.atomic_spk = false==cfg.external_spk_detail.omit_atomic_spk;
      cfg.external_speaker.hat_spk = false==cfg.external_spk_detail.omit_spk_hat;
    }

    if (cfg.internal_spk || cfg.external_speaker_value)
    {
      // set default speaker gain.
      spk_cfg.magnification = 16;
#if defined SOC_I2S_NUM
      spk_cfg.i2s_port = (i2s_port_t)(SOC_I2S_NUM - 1);
#else
      spk_cfg.i2s_port = (i2s_port_t)(I2S_NUM_MAX - 1);
#endif
      switch (_board)
      {
#if defined (M5UNIFIED_PC_BUILD)
#elif defined (CONFIG_IDF_TARGET_ESP32P4)
      case board_t::board_M5Tab5:
        if (cfg.internal_spk)
        {
          spk_cfg.pin_mck = GPIO_NUM_30;
          spk_cfg.pin_bck = GPIO_NUM_27;
          spk_cfg.pin_ws = GPIO_NUM_29;
//        spk_cfg.pin_data_in = GPIO_NUM_28;
          spk_cfg.pin_data_out = GPIO_NUM_26;
          spk_cfg.magnification = 4;
          spk_cfg.i2s_port = I2S_NUM_0;
          spk_enable_cb = _speaker_enabled_cb_tab5;
        }
        break;

#elif defined (CONFIG_IDF_TARGET_ESP32S3)
      case board_t::board_M5StackCoreS3:
      case board_t::board_M5StackCoreS3SE:
        if (cfg.internal_spk)
        {
          spk_cfg.pin_bck = GPIO_NUM_34;
          spk_cfg.pin_ws = GPIO_NUM_33;
          spk_cfg.pin_data_out = GPIO_NUM_13;
          spk_cfg.magnification = 4;
          spk_cfg.i2s_port = I2S_NUM_1;
          spk_enable_cb = _speaker_enabled_cb_cores3;
        }
        break;

      case board_t::board_M5AtomS3:
      case board_t::board_M5AtomS3Lite:
      case board_t::board_M5AtomS3R:
      case board_t::board_M5AtomS3RCam:
      case board_t::board_M5AtomS3RExt:
        if (cfg.external_speaker.atomic_spk || cfg.external_speaker.atomic_echo)
        { // for ATOMIC SPK / ATOMIC ECHO BASE
          bool atomdisplay = false;
          for (int i = 0; i < getDisplayCount(); ++i) {
            if (Displays(i).getBoard() == board_t::board_M5AtomDisplay) {
              atomdisplay = true;
              break;
            }
          }
          if (!atomdisplay) {
            bool flg_atomic_spk = false;
            if (cfg.external_speaker.atomic_spk) {
              m5gfx::pinMode(GPIO_NUM_6, m5gfx::pin_mode_t::input_pulldown); // MOSI
              m5gfx::pinMode(GPIO_NUM_7, m5gfx::pin_mode_t::input_pulldown); // SCLK
              if (m5gfx::gpio_in(GPIO_NUM_6)
                && m5gfx::gpio_in(GPIO_NUM_7))
              {
                flg_atomic_spk = true;
                ESP_LOGD("M5Unified", "ATOMIC SPK");
                // atomic_spkのSDカード用ピンを割当
                _get_pin_table[sd_spi_sclk] = GPIO_NUM_7;
                _get_pin_table[sd_spi_copi] = GPIO_NUM_6;
                _get_pin_table[sd_spi_cipo] = GPIO_NUM_8;
                cfg.internal_imu = false; /// avoid conflict with i2c
                cfg.internal_rtc = false; /// avoid conflict with i2c
                spk_cfg.pin_bck = GPIO_NUM_5;
                spk_cfg.pin_ws = GPIO_NUM_39;
                spk_cfg.pin_data_out = GPIO_NUM_38;
                spk_cfg.magnification = 16;
              }
            }
            if (cfg.external_speaker.atomic_echo && !flg_atomic_spk) {
              spk_cfg.pin_bck = GPIO_NUM_8;
              spk_cfg.pin_ws = GPIO_NUM_6;
              spk_cfg.pin_data_out = GPIO_NUM_5;
              spk_cfg.magnification = 1;
              spk_enable_cb = _speaker_enabled_cb_atomic_echo;

              mic_cfg.i2s_port = spk_cfg.i2s_port;
              mic_cfg.pin_bck = GPIO_NUM_8;
              mic_cfg.pin_ws = GPIO_NUM_6;
              mic_cfg.pin_data_in = GPIO_NUM_7;
              mic_cfg.magnification = 1;
              mic_cfg.over_sampling = 1;
              mic_cfg.pin_mck = GPIO_NUM_NC;
              mic_cfg.stereo = false;
              mic_enable_cb = _microphone_enabled_cb_atomic_echo;
            }
          }
        }
        break;

      case board_t::board_M5Capsule:
        if (cfg.internal_spk)
        {
          spk_cfg.pin_data_out = GPIO_NUM_2;
          spk_cfg.buzzer = true;
          spk_cfg.magnification = 48;
        }
        break;

      case board_t::board_M5Dial:
      case board_t::board_M5DinMeter:
        if (cfg.internal_spk)
        {
          spk_cfg.pin_data_out = GPIO_NUM_3;
          spk_cfg.buzzer = true;
          spk_cfg.magnification = 48;
        }
        break;

      case board_t::board_M5AirQ:
        if (cfg.internal_spk)
        {
          spk_cfg.pin_data_out = GPIO_NUM_9;
          spk_cfg.buzzer = true;
          spk_cfg.magnification = 48;
        }
        break;

      case board_t::board_M5VAMeter:
        if (cfg.internal_spk)
        {
          spk_cfg.pin_data_out = GPIO_NUM_14;
          spk_cfg.buzzer = true;
          spk_cfg.magnification = 48;
        }
        break;

      case board_t::board_M5PaperS3:
        if (cfg.internal_spk)
        {
          spk_cfg.pin_data_out = GPIO_NUM_21;
          spk_cfg.buzzer = true;
          spk_cfg.magnification = 48;
        }
        break;

      case board_t::board_M5Cardputer:
        if (cfg.internal_spk)
        {
          spk_cfg.pin_bck = GPIO_NUM_41;
          spk_cfg.pin_ws = GPIO_NUM_43;
          spk_cfg.pin_data_out = GPIO_NUM_42;
          spk_cfg.magnification = 16;
          spk_cfg.i2s_port = I2S_NUM_1;
        }
        break;

      case board_t::board_M5StampPLC:
        if (cfg.internal_spk)
        {
          spk_cfg.pin_data_out = GPIO_NUM_44;
          spk_cfg.buzzer = true;
          spk_cfg.magnification = 48;
        }
        break;

#elif !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
      case board_t::board_M5Stack:
        if (cfg.internal_spk)
        {
          m5gfx::gpio_lo(GPIO_NUM_25);
          m5gfx::pinMode(GPIO_NUM_25, m5gfx::pin_mode_t::output);
          spk_cfg.i2s_port = I2S_NUM_0;
          spk_cfg.use_dac = true;
          spk_cfg.pin_data_out = GPIO_NUM_25;
          spk_cfg.magnification = 8;
          spk_cfg.sample_rate *= 2;
        }
        break;

      case board_t::board_M5StackCoreInk:
      case board_t::board_M5StickCPlus:
      case board_t::board_M5StickCPlus2:
        if (cfg.internal_spk)
        {
          spk_cfg.buzzer = true;
          spk_cfg.pin_data_out = GPIO_NUM_2;
          spk_cfg.magnification = 48;
        }
        NON_BREAK;

      case board_t::board_M5StickC:
        if (cfg.external_speaker.hat_spk2 && (_board != board_t::board_M5StackCoreInk))
        { /// for HAT SPK2 (for StickC/StickCPlus.  CoreInk does not support.)
          spk_cfg.pin_data_out = GPIO_NUM_25;
          spk_cfg.pin_bck = GPIO_NUM_26;
          spk_cfg.pin_ws = GPIO_NUM_0;
          spk_cfg.i2s_port = I2S_NUM_1;
          spk_cfg.use_dac = false;
          spk_cfg.buzzer = false;
          spk_cfg.magnification = 16;
        }
        else if (cfg.external_speaker.hat_spk)
        { /// for HAT SPK
          use_hat_spk = true;
          gpio_num_t pin_en = _board == board_t::board_M5StackCoreInk ? GPIO_NUM_25 : GPIO_NUM_0;
          m5gfx::gpio_lo(pin_en);
          m5gfx::pinMode(pin_en, m5gfx::pin_mode_t::output);
          m5gfx::gpio_lo(GPIO_NUM_26);
          m5gfx::pinMode(GPIO_NUM_26, m5gfx::pin_mode_t::output);
          spk_cfg.pin_data_out = GPIO_NUM_26;
          spk_cfg.i2s_port = I2S_NUM_0;
          spk_cfg.use_dac = true;
          spk_cfg.buzzer = false;
          spk_cfg.magnification = 32;
          spk_enable_cb = _speaker_enabled_cb_hat_spk;
        }
        break;

      case board_t::board_M5Tough:
        // The magnification is set higher than Core2 here because the waterproof case reduces the sound.;
        spk_cfg.magnification = 24;
        NON_BREAK;
      case board_t::board_M5StackCore2:
        if (cfg.internal_spk)
        {
          spk_cfg.pin_bck = GPIO_NUM_12;
          spk_cfg.pin_ws = GPIO_NUM_0;
          spk_cfg.pin_data_out = GPIO_NUM_2;
          spk_enable_cb = _speaker_enabled_cb_core2;
        }
        break;

      case board_t::board_M5AtomEcho:
        if (cfg.internal_spk && (Display.getBoard() != board_t::board_M5AtomDisplay))
        { // for ATOM ECHO
          spk_cfg.pin_bck = GPIO_NUM_19;
          spk_cfg.pin_ws = GPIO_NUM_33;
          spk_cfg.pin_data_out = GPIO_NUM_22;
          spk_cfg.magnification = 12;
        }
        NON_BREAK;
      case board_t::board_M5AtomLite:
      case board_t::board_M5AtomMatrix:
      case board_t::board_M5AtomPsram:
        if (cfg.external_speaker.atomic_spk || cfg.external_speaker.atomic_echo)
        { // for ATOMIC SPK / ATOMIC ECHO BASE
          bool atomdisplay = false;
          for (int i = 0; i < getDisplayCount(); ++i) {
            if (Displays(i).getBoard() == board_t::board_M5AtomDisplay) {
              atomdisplay = true;
              break;
            }
          }
          if (!atomdisplay) {
            bool flg_atomic_spk = false;
            if (cfg.external_speaker.atomic_spk) {
              // 19,23 pulldown read check ( all high = ATOMIC_SPK ? ) // MISO is not used for judgment as it changes depending on the state of the SD card.
              gpio_num_t pin = (_board == board_t::board_M5AtomPsram) ? GPIO_NUM_5 : GPIO_NUM_23;
              m5gfx::pinMode(GPIO_NUM_19, m5gfx::pin_mode_t::input_pulldown); // MOSI
              m5gfx::pinMode(pin        , m5gfx::pin_mode_t::input_pulldown); // SCLK
              if (m5gfx::gpio_in(GPIO_NUM_19)
                && m5gfx::gpio_in(pin        ))
              {
                flg_atomic_spk = true;
                ESP_LOGD("M5Unified", "ATOMIC SPK");
                // atomic_spkのSDカード用ピンを割当
                _get_pin_table[sd_spi_sclk] = pin;
                _get_pin_table[sd_spi_copi] = GPIO_NUM_19;
                _get_pin_table[sd_spi_cipo] = GPIO_NUM_33;
                cfg.internal_imu = false; /// avoid conflict with i2c
                cfg.internal_rtc = false; /// avoid conflict with i2c
                spk_cfg.pin_bck = GPIO_NUM_22;
                spk_cfg.pin_ws = GPIO_NUM_21;
                spk_cfg.pin_data_out = GPIO_NUM_25;
                spk_cfg.magnification = 16;
                auto mic = Mic.config();
                mic.pin_data_in = -1;   // disable mic for ATOMECHO
                Mic.config(mic);
              }
            }
            if (cfg.external_speaker.atomic_echo && !flg_atomic_spk) {
              spk_cfg.pin_bck = GPIO_NUM_33;
              spk_cfg.pin_ws = GPIO_NUM_19;
              spk_cfg.pin_data_out = GPIO_NUM_22;
              spk_cfg.magnification = 1;
              spk_enable_cb = _speaker_enabled_cb_atomic_echo;

              mic_cfg.i2s_port = spk_cfg.i2s_port;
              mic_cfg.pin_bck = GPIO_NUM_33;
              mic_cfg.pin_ws = GPIO_NUM_19;
              mic_cfg.pin_data_in = GPIO_NUM_23;
              mic_cfg.magnification = 1;
              mic_cfg.over_sampling = 1;
              mic_cfg.pin_mck = GPIO_NUM_NC;
              mic_cfg.stereo = false;
              mic_enable_cb = _microphone_enabled_cb_atomic_echo;
            }
          }
        }
        break;
#endif
      default:
        break;
      }

      if (cfg.external_speaker_value)
      {
#if defined (M5UNIFIED_PC_BUILD)
#elif defined ( CONFIG_IDF_TARGET_ESP32P4 )
 #define ENABLE_M5MODULE
        if (_board == board_t::board_M5Tab5)
#elif defined ( CONFIG_IDF_TARGET_ESP32S3 )
 #define ENABLE_M5MODULE
        if (_board == board_t::board_M5StackCoreS3
         || _board == board_t::board_M5StackCoreS3SE)
#elif defined ( CONFIG_IDF_TARGET_ESP32 ) || !defined ( CONFIG_IDF_TARGET )
 #define ENABLE_M5MODULE
        if (  _board == board_t::board_M5Stack
          || _board == board_t::board_M5StackCore2
          || _board == board_t::board_M5Tough)
#endif
        {
#ifdef ENABLE_M5MODULE
          bool use_module_display = cfg.external_speaker.module_display
                                && (0 <= getDisplayIndex(m5gfx::board_M5ModuleDisplay));
          if (use_module_display || cfg.external_speaker.module_rca)
          {
            if (use_module_display) {
              spk_cfg.sample_rate = 48000; // Module Display audio output is fixed at 48 kHz
            }
            // ModuleDisplay or Module RCA
            spk_cfg.pin_bck      = getPin(use_module_display ? pin_name_t::mbus_pin21 : pin_name_t::mbus_pin22);
            spk_cfg.pin_data_out = getPin(pin_name_t::mbus_pin23);
            spk_cfg.pin_ws       = getPin(pin_name_t::mbus_pin24);     // LRCK

            spk_cfg.i2s_port = I2S_NUM_1;
            spk_cfg.magnification = 16;
            spk_cfg.stereo = true;
            spk_cfg.buzzer = false;
            spk_cfg.use_dac = false;
            spk_enable_cb = nullptr;
          }
 #undef ENABLE_M5MODULE
#endif
        }
      }
    }
    if (mic_cfg.pin_data_in >= 0)
    {
      Mic.setCallback(this, mic_enable_cb);
      Mic.config(mic_cfg);
    }
    if (spk_cfg.pin_data_out >= 0)
    {
      Speaker.setCallback(this, spk_enable_cb);
      Speaker.config(spk_cfg);
    }
  }

  bool M5Unified::_begin_rtc_imu(const config_t& cfg)
  {
    bool port_a_used = false;
    if (cfg.external_rtc || cfg.external_imu)
    {
      M5.Ex_I2C.begin();
    }

    if (cfg.internal_rtc && In_I2C.isEnabled())
    {
      M5.Rtc.begin();
    }
    if (!M5.Rtc.isEnabled() && cfg.external_rtc)
    {
      port_a_used = M5.Rtc.begin(&M5.Ex_I2C);
    }
    if (M5.Rtc.isEnabled())
    {
      M5.Rtc.setSystemTimeFromRtc();
      if (cfg.disable_rtc_irq) {
        M5.Rtc.disableIRQ();
      }
    }

    if (cfg.internal_imu && In_I2C.isEnabled())
    {
      M5.Imu.begin(&M5.In_I2C, M5.getBoard());
    }
    if (!M5.Imu.isEnabled() && cfg.external_imu)
    {
      port_a_used = M5.Imu.begin(&M5.Ex_I2C) || port_a_used;
    }
    return port_a_used;
  }

  void M5Unified::update( void )
  {
    auto ms = m5gfx::millis();
    _updateMsec = ms;

    // 1=BtnA / 2=BtnB / 4=BtnC / 8=BtnEXT / 16=BtnPWR
    uint_fast8_t use_rawstate_bits = 0;
    uint_fast8_t btn_rawstate_bits = 0;

    if (Touch.isEnabled())
    {
      Touch.update(ms);

      int tb_y = 0;
      int tb_k = 0;
      switch (_board)
      {
      case board_t::board_M5StackCore2:
      case board_t::board_M5Tough:
      case board_t::board_M5StackCoreS3SE:
      case board_t::board_M5StackCoreS3:
        tb_y = 240;
        tb_k = 614; // (65536*3/320)
        break;
      case board_t::board_M5Paper:
      case board_t::board_M5PaperS3:
        tb_y = 960;
        tb_k = 364; // (65536*3/540)
        break;
      case board_t::board_M5Tab5:
        tb_y = 1280;
        tb_k = 273; // (65536*3/540)
        break;
      default:
        break;
      }

      if (tb_k)
      {
          tb_y -= _touch_button_height;
          if (tb_y < 0) { tb_y = 0; }

          use_rawstate_bits = 0b00111;
          int i = Touch.getCount();
          while (--i >= 0)
          {
            auto raw = Touch.getTouchPointRaw(i);
            if (raw.y >= tb_y)
            {
              auto det = Touch.getDetail(i);
              if (det.state & touch_state_t::touch)
              {
                if (BtnA.isPressed()) { btn_rawstate_bits |= 1 << 0; }
                if (BtnB.isPressed()) { btn_rawstate_bits |= 1 << 1; }
                if (BtnC.isPressed()) { btn_rawstate_bits |= 1 << 2; }
                if (btn_rawstate_bits || !(det.state & touch_state_t::mask_moving))
                {
                  btn_rawstate_bits |= 1 << ((raw.x * tb_k) >> 16);
                }
              }
            }
          }
      }
    }

#if defined (M5UNIFIED_PC_BUILD)
    use_rawstate_bits = 0b10111;
    btn_rawstate_bits = (!m5gfx::gpio_in(39) ? 0b00001 : 0) // LEFT=BtnA
                      | (!m5gfx::gpio_in(38) ? 0b00010 : 0) // DOWN=BtnB
                      | (!m5gfx::gpio_in(37) ? 0b00100 : 0) // RIGHT=BtnC
                      | (!m5gfx::gpio_in(36) ? 0b10000 : 0) // UP=BtnPWR
                      ;
#elif !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)

    uint_fast8_t raw_gpio32_39 = ~GPIO.in1.data;
    switch (_board)
    {
    case board_t::board_M5StackCoreInk:
      {
        uint32_t raw_gpio0_31 = ~GPIO.in;
        use_rawstate_bits = 0b11000;
        btn_rawstate_bits = (((raw_gpio0_31 >> CoreInk_BUTTON_EXT_PIN) & 1) << 3)
                          | (((raw_gpio0_31 >> CoreInk_BUTTON_PWR_PIN) & 1) << 4);
      }
      NON_BREAK; /// don't break;

    case board_t::board_M5Paper:
    case board_t::board_M5Station:
      use_rawstate_bits |= 0b00111;
      btn_rawstate_bits |= (raw_gpio32_39 >> (GPIO_NUM_37 & 31)) & 0x07; // gpio37 A / gpio38 B / gpio39 C
      break;

    case board_t::board_M5Stack:
      use_rawstate_bits = 0b00111;
      btn_rawstate_bits = (((raw_gpio32_39 >> (GPIO_NUM_38 & 31)) & 1) << 1)  // gpio38 B
                        | (((raw_gpio32_39 >> (GPIO_NUM_37 & 31)) & 1) << 2); // gpio37 C
      NON_BREAK; /// don't break;

    case board_t::board_M5AtomLite:
    case board_t::board_M5AtomMatrix:
    case board_t::board_M5AtomEcho:
    case board_t::board_M5AtomPsram:
    case board_t::board_M5AtomU:
    case board_t::board_M5StampPico:
      use_rawstate_bits |= 0b00001;
      btn_rawstate_bits |= (raw_gpio32_39 >> (GPIO_NUM_39 & 31)) & 1; // gpio39 A
      break;

    case board_t::board_M5StickCPlus2:
      use_rawstate_bits = 0b10000;
      btn_rawstate_bits = (((raw_gpio32_39 >> (GPIO_NUM_35 & 31)) & 1)<<4); // gpio35 PWR
      NON_BREAK; /// don't break;

    case board_t::board_M5StickC:
    case board_t::board_M5StickCPlus:
      use_rawstate_bits |= 0b00011;
      btn_rawstate_bits |= (( raw_gpio32_39 >> (GPIO_NUM_37 & 31)) & 1    )  // gpio37 A
                         | (((raw_gpio32_39 >> (GPIO_NUM_39 & 31)) & 1)<<1); // gpio39 B
      break;

    default:
      break;
    }

#elif defined (CONFIG_IDF_TARGET_ESP32S3)

    switch (_board)
    {
    case board_t::board_M5AirQ:
      use_rawstate_bits = 0b00011;
      btn_rawstate_bits = ((!m5gfx::gpio_in(GPIO_NUM_0)) & 1)
                        | ((!m5gfx::gpio_in(GPIO_NUM_8)) & 1) << 1;
      break;

    case board_t::board_M5VAMeter:
      use_rawstate_bits = 0b00011;
      btn_rawstate_bits = ((!m5gfx::gpio_in(GPIO_NUM_2)) & 1)
                        | ((!m5gfx::gpio_in(GPIO_NUM_0)) & 1) << 1;
      break;

    case board_t::board_M5StampS3:
    case board_t::board_M5Cardputer:
      use_rawstate_bits = 0b00001;
      btn_rawstate_bits = (!m5gfx::gpio_in(GPIO_NUM_0)) & 1;
      break;

    case board_t::board_M5AtomS3:
    case board_t::board_M5AtomS3Lite:
    case board_t::board_M5AtomS3U:
    case board_t::board_M5AtomS3R:
      use_rawstate_bits = 0b00001;
      btn_rawstate_bits = (!m5gfx::gpio_in(GPIO_NUM_41)) & 1;
      break;

    case board_t::board_M5Capsule:
    case board_t::board_M5Dial:
    case board_t::board_M5DinMeter:
      use_rawstate_bits = 0b00001;
      btn_rawstate_bits = (!m5gfx::gpio_in(GPIO_NUM_42)) & 1;
      break;

    case board_t::board_M5StampPLC:
    {
      use_rawstate_bits = 0b00111;
      auto value = _io_expander_a->readRegister8(0x0F);
      btn_rawstate_bits = (!(value & 0b100) ? 0b00001 : 0) // BtnA
                        | (!(value & 0b010) ? 0b00010 : 0) // BtnB
                        | (!(value & 0b001) ? 0b00100 : 0) // BtnC
                        ;
    }
      break;

    default:

    break;
    }

#elif defined (CONFIG_IDF_TARGET_ESP32C3)

    switch (_board)
    {
    case board_t::board_M5StampC3:
      use_rawstate_bits = 0b00001;
      btn_rawstate_bits = (!m5gfx::gpio_in(GPIO_NUM_3)) & 1;
      break;

    case board_t::board_M5StampC3U:
      use_rawstate_bits = 0b00001;
      btn_rawstate_bits = (!m5gfx::gpio_in(GPIO_NUM_9)) & 1;
      break;

    default:
      break;
    }

#elif defined (CONFIG_IDF_TARGET_ESP32C6)

    switch (_board)
    {
    case board_t::board_M5NanoC6:
      use_rawstate_bits = 0b00001;
      btn_rawstate_bits = (!m5gfx::gpio_in(GPIO_NUM_9) ? 0b00001 : 0);
      break;

    default:
      break;
    }

#endif

    if (use_rawstate_bits) {
      for (int i = 0; i < 5; ++i) {
        if (use_rawstate_bits & (1 << i)) {
          _buttons[i].setRawState(ms, btn_rawstate_bits & (1 << i));
        }
      }
    }

#if defined (CONFIG_IDF_TARGET_ESP32) || defined (CONFIG_IDF_TARGET_ESP32S3)
    if (use_pmic_button)
    {
      Button_Class::button_state_t state = Button_Class::button_state_t::state_nochange;
      bool read_axp = (ms - BtnPWR.getUpdateMsec()) >= BTNPWR_MIN_UPDATE_MSEC;
      if (read_axp || BtnPWR.getState())
      {
        switch (Power.getKeyState())
        {
        case 0: break;
        case 2:   state = Button_Class::button_state_t::state_clicked; break;
        default:  state = Button_Class::button_state_t::state_hold;    break;
        }
        BtnPWR.setState(ms, state);
      }
    }
#endif
  }

  void M5Unified::setTouchButtonHeightByRatio(uint8_t ratio)
  {
    uint32_t height = 0;
    switch (_board)
    {
    case board_t::board_M5StackCore2:
    case board_t::board_M5Tough:
    case board_t::board_M5StackCoreS3SE:
    case board_t::board_M5StackCoreS3:
      height = 240;
      break;
    case board_t::board_M5Paper:
    case board_t::board_M5PaperS3:
      height = 960;
      break;
    case board_t::board_M5Tab5:
      height = 1280;
      break;
    default:
      break;
    }
    _touch_button_height = height * ratio / 255;
  }

  M5GFX& M5Unified::getDisplay(size_t index)
  {
    return index != _primary_display_index && index < this->_displays.size() ? this->_displays[index] : Display;
  }

  std::size_t M5Unified::addDisplay(M5GFX& dsp)
  {
    this->_displays.push_back(dsp);
    auto res = this->_displays.size() - 1;
    setPrimaryDisplay(res == 0 ? 0 : _primary_display_index);

    // Touch screen operation is always limited to the first display.
    Touch.begin(_displays.front().touch() ? &_displays.front() : nullptr);

    return res;
  }

  int32_t M5Unified::getDisplayIndex(m5gfx::board_t board) {
    int i = 0;
    for (auto &d : _displays)
    {
      if (board == d.getBoard()) { return i; }
      ++i;
    }
    return -1;
  }

  int32_t M5Unified::getDisplayIndex(std::initializer_list<m5gfx::board_t> board_list)
  {
    for (auto b : board_list)
    {
      int32_t i = getDisplayIndex(b);
      if (i >= 0) { return i; }
    }
    return -1;
  }

  bool M5Unified::setPrimaryDisplay(std::size_t index)
  {
    if (index >= _displays.size()) { return false; }
    std::size_t pdi = _primary_display_index;

    if (pdi < _displays.size())
    {
      _displays[pdi] = Display;
    }
    _primary_display_index = index;
    Display = _displays[index];
    return true;
  }

  bool M5Unified::setPrimaryDisplayType(std::initializer_list<m5gfx::board_t> board_list)
  {
    auto i = getDisplayIndex(board_list);
    bool res = (i >= 0);
    if (res) { setPrimaryDisplay(i); }
    return res;
  }

  void M5Unified::setLogDisplayIndex(size_t index)
  {
    Log.setDisplay(getDisplay(index));
  }

  void M5Unified::setLogDisplayType(std::initializer_list<m5gfx::board_t> board_list)
  {
    auto i = getDisplayIndex(board_list);
    if (i >= 0) { Log.setDisplay(getDisplay(i)); }
  }
}
