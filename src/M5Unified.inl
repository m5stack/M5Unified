// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#ifndef M5UNIFIED_IMPLEMENTATION
#error "M5Unified.inl is part of M5Unified.cpp and is not meant to be included on its own"
#endif

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include "M5Unified.hpp"
#include "utility/m5unified_i2c_addr.hpp"
#include "utility/PI4IOE5V6408_Class.hpp"
#include "utility/M5IOE1_Class.hpp"

#if !defined (M5UNIFIED_PC_BUILD)
#include <soc/soc.h>
#include <soc/efuse_reg.h>
#include <soc/gpio_periph.h>
#if __has_include (<soc/io_mux_reg.h>)
#include <soc/io_mux_reg.h>
#endif

#if defined (CONFIG_IDF_TARGET_ESP32P4)
#include <esp_chip_info.h>
#endif

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

#include "utility/m5unified_i2s.h"

#include "utility/led/LED_Strip_Class.hpp"
#include "utility/led/LED_PMIC_Class.hpp"
#include "utility/led/LED_PowerHub_Class.hpp"
#include "utility/led/LED_PaperMono_Class.hpp"

#if defined (ARDUINO) && defined (CONFIG_IDF_TARGET_ESP32P4)
// ESP-Hosted (Arduino core HAL) の SDIO ピン設定。弱参照にして、WiFi/BLE を使わないビルドには
// ESP-Hosted のコードを引き込まない (リンカが解決しなければ nullptr になる)。
extern "C" bool hostedSetPins(int8_t clk, int8_t cmd, int8_t d0, int8_t d1, int8_t d2, int8_t d3, int8_t rst) __attribute__((weak));
extern "C" bool hostedIsInitialized(void) __attribute__((weak));
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
// Pin number table. Place unknown at the end of the table. 
// If there is no corresponding value, the value of unknown is used.
// ピン番号テーブル。 unknownをテーブルの最後に配置する。該当が無い場合はunknownの値が使用される。
static constexpr const uint8_t _pin_table_i2c_ex_in[][5] = {
                            // In SCL,SDA, EX SCL,SDA
#if defined (CONFIG_IDF_TARGET_ESP32S3)
{ board_t::board_M5StackCoreS3, GPIO_NUM_11,GPIO_NUM_12 , GPIO_NUM_1 ,GPIO_NUM_2  },
{ board_t::board_M5StackCoreS3SE,GPIO_NUM_11,GPIO_NUM_12, GPIO_NUM_1 ,GPIO_NUM_2  },
{ board_t::board_M5StackChan  , GPIO_NUM_11, GPIO_NUM_12, GPIO_NUM_1, GPIO_NUM_2  },
{ board_t::board_M5StickS3    , GPIO_NUM_48,GPIO_NUM_47 , GPIO_NUM_10,GPIO_NUM_9  },
{ board_t::board_M5StampS3    , 255        ,255         , GPIO_NUM_15,GPIO_NUM_13 },
{ board_t::board_M5DualKey    , 255        ,255         , 255        ,255         }, // No wired I2C pins.
{ board_t::board_M5Capsule    , GPIO_NUM_10,GPIO_NUM_8  , GPIO_NUM_15,GPIO_NUM_13 },
{ board_t::board_M5Dial       , GPIO_NUM_12,GPIO_NUM_11 , GPIO_NUM_15,GPIO_NUM_13 },
{ board_t::board_M5DinMeter   , GPIO_NUM_12,GPIO_NUM_11 , GPIO_NUM_15,GPIO_NUM_13 },
{ board_t::board_M5AirQ       , GPIO_NUM_12,GPIO_NUM_11 , GPIO_NUM_15,GPIO_NUM_13 },
{ board_t::board_M5Cardputer  , 255        ,255         , GPIO_NUM_1 ,GPIO_NUM_2  },
{ board_t::board_M5CardputerADV,GPIO_NUM_9 ,GPIO_NUM_8  , GPIO_NUM_1 ,GPIO_NUM_2  },
{ board_t::board_M5VAMeter    , GPIO_NUM_6 ,GPIO_NUM_5  , GPIO_NUM_9 ,GPIO_NUM_8  },
{ board_t::board_M5AtomS3R    , GPIO_NUM_0 ,GPIO_NUM_45 , GPIO_NUM_1 ,GPIO_NUM_2  },
{ board_t::board_M5AtomS3RExt , GPIO_NUM_0 ,GPIO_NUM_45 , GPIO_NUM_1 ,GPIO_NUM_2  },
{ board_t::board_M5AtomVoiceS3R,GPIO_NUM_0 ,GPIO_NUM_45 , GPIO_NUM_1 ,GPIO_NUM_2  },
{ board_t::board_M5AtomS3RCam , GPIO_NUM_0 ,GPIO_NUM_45 , GPIO_NUM_1 ,GPIO_NUM_2  },
{ board_t::board_M5PaperS3    , GPIO_NUM_42,GPIO_NUM_41 , GPIO_NUM_1 ,GPIO_NUM_2  },
{ board_t::board_M5PaperDIY   , GPIO_NUM_42,GPIO_NUM_41 , GPIO_NUM_42,GPIO_NUM_41 },
{ board_t::board_M5StampPLC   , GPIO_NUM_15,GPIO_NUM_13 , GPIO_NUM_1 ,GPIO_NUM_2  },
{ board_t::board_M5PowerHub   , GPIO_NUM_48,GPIO_NUM_45 , GPIO_NUM_16,GPIO_NUM_15 },
{ board_t::board_M5StampS3Bat , GPIO_NUM_47,GPIO_NUM_48 , 255        ,255         },
{ board_t::board_M5PaperColor , GPIO_NUM_2 ,GPIO_NUM_3  , GPIO_NUM_5 ,GPIO_NUM_4  },
{ board_t::board_M5ChainCaptain,GPIO_NUM_2 ,GPIO_NUM_3  , GPIO_NUM_6 ,GPIO_NUM_7  },
{ board_t::board_M5PaperMono  , GPIO_NUM_48,GPIO_NUM_47 , 255        ,255         },
{ board_t::board_M5StopWatch  , GPIO_NUM_48,GPIO_NUM_47 , GPIO_NUM_11,GPIO_NUM_10 },
{ board_t::board_unknown      , GPIO_NUM_39,GPIO_NUM_38 , GPIO_NUM_1 ,GPIO_NUM_2  }, // AtomS3,AtomS3Lite,AtomS3U
#elif defined (CONFIG_IDF_TARGET_ESP32C3)
{ board_t::board_unknown      , 255        ,255         , GPIO_NUM_0 ,GPIO_NUM_1  },
#elif defined (CONFIG_IDF_TARGET_ESP32C6)
{ board_t::board_M5UnitC6L     ,GPIO_NUM_8 ,GPIO_NUM_10 , 255        ,255         },
{ board_t::board_ArduinoNessoN1,GPIO_NUM_8 ,GPIO_NUM_10 , GPIO_NUM_8 ,GPIO_NUM_10 },
{ board_t::board_M5NanoC6     , 255        ,255         , GPIO_NUM_1 ,GPIO_NUM_2  },
{ board_t::board_unknown      , 255        ,255         , 255        ,255         },
#elif defined (CONFIG_IDF_TARGET_ESP32C61)
{ board_t::board_M5CoreMatrix , GPIO_NUM_1 ,GPIO_NUM_0  , GPIO_NUM_1 ,GPIO_NUM_0  }, // Grove shares the internal bus (level-shifted)
{ board_t::board_unknown      , 255        ,255         , 255        ,255         },
#elif defined (CONFIG_IDF_TARGET_ESP32H2)
{ board_t::board_M5NanoH2     , 255        ,255         , GPIO_NUM_1 ,GPIO_NUM_2  },
{ board_t::board_unknown      , 255        ,255         , 255        ,255         },
#elif defined (CONFIG_IDF_TARGET_ESP32P4)
{ board_t::board_M5CoreP4X    , GPIO_NUM_9 ,GPIO_NUM_11 , GPIO_NUM_16,GPIO_NUM_18 }, // CoreP4X
{ board_t::board_M5Tab5       , GPIO_NUM_32,GPIO_NUM_31 , GPIO_NUM_54,GPIO_NUM_53 }, // Tab5
{ board_t::board_M5UnitPoEP4  , GPIO_NUM_1 ,GPIO_NUM_0  , GPIO_NUM_54,GPIO_NUM_53 },
{ board_t::board_unknown      , 255        ,255         , 255        ,255         },
#elif defined (CONFIG_IDF_TARGET_ESP32C5)
{ board_t::board_M5StampC5    , 255        ,255         , 255        ,255         },
{ board_t::board_M5ToughC5    , GPIO_NUM_3 ,GPIO_NUM_2  , GPIO_NUM_3 ,GPIO_NUM_2  }, // PortA は内部バスと同一 (レベルシフタ経由の物理分配)
{ board_t::board_unknown      , 255        ,255         , 255        ,255         },
#elif !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
{ board_t::board_M5Stack      , GPIO_NUM_22,GPIO_NUM_21 , GPIO_NUM_22,GPIO_NUM_21 },
{ board_t::board_M5Paper      , GPIO_NUM_22,GPIO_NUM_21 , GPIO_NUM_32,GPIO_NUM_25 },
{ board_t::board_M5TimerCam   , GPIO_NUM_14,GPIO_NUM_12 , GPIO_NUM_13,GPIO_NUM_4  },
{ board_t::board_M5AtomLite   , GPIO_NUM_21,GPIO_NUM_25 , GPIO_NUM_32,GPIO_NUM_26 },
{ board_t::board_M5AtomMatrix , GPIO_NUM_21,GPIO_NUM_25 , GPIO_NUM_32,GPIO_NUM_26 },
{ board_t::board_M5AtomVoice  , GPIO_NUM_21,GPIO_NUM_25 , GPIO_NUM_32,GPIO_NUM_26 },
{ board_t::board_M5AtomU      , GPIO_NUM_21,GPIO_NUM_25 , GPIO_NUM_32,GPIO_NUM_26 },
{ board_t::board_M5AtomPsram  , GPIO_NUM_21,GPIO_NUM_25 , GPIO_NUM_32,GPIO_NUM_26 },
{ board_t::board_unknown      , GPIO_NUM_22,GPIO_NUM_21 , GPIO_NUM_33,GPIO_NUM_32 }, // Core2,Tough,StickC,CoreInk,Station,StampPico
#else
// A chip without boards of its own: nothing to probe. Kept explicit so that it never
// inherits the ESP32 table above.
{ board_t::board_unknown      , 255        ,255         , 255        ,255         },
#endif
};

static constexpr const uint8_t _pin_table_port_bc[][5] = {
                          //pB p1,p2, pC p1,p2 (p1 close to 5V)
#if defined (CONFIG_IDF_TARGET_ESP32S3)
{ board_t::board_M5StackCoreS3, GPIO_NUM_8 ,GPIO_NUM_9 , GPIO_NUM_18,GPIO_NUM_17 },
{ board_t::board_M5StackCoreS3SE,GPIO_NUM_8,GPIO_NUM_9 , GPIO_NUM_18,GPIO_NUM_17 },
{ board_t::board_M5StackChan  , GPIO_NUM_8, GPIO_NUM_9, GPIO_NUM_18, GPIO_NUM_17 },
{ board_t::board_M5Dial       , GPIO_NUM_1 ,GPIO_NUM_2 , 255        ,255         },
{ board_t::board_M5DinMeter   , GPIO_NUM_1 ,GPIO_NUM_2 , 255        ,255         },
{ board_t::board_M5PowerHub   , 255        ,       255 , GPIO_NUM_1 ,GPIO_NUM_2  },
{ board_t::board_M5ChainCaptain,GPIO_NUM_17,GPIO_NUM_18, 255        ,255         },
#elif defined (CONFIG_IDF_TARGET_ESP32C3)
#elif defined (CONFIG_IDF_TARGET_ESP32C6)
{ board_t::board_M5UnitC6L     ,GPIO_NUM_4 ,GPIO_NUM_5 , GPIO_NUM_4 ,GPIO_NUM_5  },
{ board_t::board_ArduinoNessoN1,GPIO_NUM_4 ,GPIO_NUM_5 , GPIO_NUM_4 ,GPIO_NUM_5  },
#elif defined (CONFIG_IDF_TARGET_ESP32C61)
#elif defined (CONFIG_IDF_TARGET_ESP32H2)
#elif defined (CONFIG_IDF_TARGET_ESP32P4)
{ board_t::board_M5Tab5       , GPIO_NUM_17,GPIO_NUM_52, GPIO_NUM_7 ,GPIO_NUM_6  }, // Tab5
#elif defined (CONFIG_IDF_TARGET_ESP32C5)
{ board_t::board_M5ToughC5    , GPIO_NUM_1 ,GPIO_NUM_6  , GPIO_NUM_12,GPIO_NUM_11 },
#elif !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
{ board_t::board_M5Stack      , GPIO_NUM_36,GPIO_NUM_26 , GPIO_NUM_16,GPIO_NUM_17 },
{ board_t::board_M5StackCore2 , GPIO_NUM_36,GPIO_NUM_26 , GPIO_NUM_13,GPIO_NUM_14 },
{ board_t::board_M5Tough      , GPIO_NUM_36,GPIO_NUM_26 , GPIO_NUM_13,GPIO_NUM_14 },
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
{ board_t::board_M5StackChan, 14, 10, 18, 17 },
#elif defined (CONFIG_IDF_TARGET_ESP32C3)
#elif defined (CONFIG_IDF_TARGET_ESP32C6)
#elif defined (CONFIG_IDF_TARGET_ESP32C61)
#elif defined (CONFIG_IDF_TARGET_ESP32H2)
#elif defined (CONFIG_IDF_TARGET_ESP32C5)
#elif !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
{ board_t::board_M5Stack      , GPIO_NUM_34,GPIO_NUM_35 , GPIO_NUM_5 ,GPIO_NUM_13 },
{ board_t::board_M5StackCore2 , GPIO_NUM_34,GPIO_NUM_35 , GPIO_NUM_27,GPIO_NUM_19 },
{ board_t::board_M5Station    , GPIO_NUM_36,GPIO_NUM_26 , GPIO_NUM_16,GPIO_NUM_17 }, // B2 / C2
#endif
{ board_t::board_unknown      , 255        ,255         , 255        ,255         },
};

static constexpr const uint8_t _pin_table_sd[][7] = {
                            // clk,cmd(MOSI),D0(MISO),D1,D2,D3(CS)
#if defined (CONFIG_IDF_TARGET_ESP32S3)
{ board_t::board_M5StackCoreS3, GPIO_NUM_36, GPIO_NUM_37, GPIO_NUM_35, 255        , 255       , GPIO_NUM_4  },
{ board_t::board_M5StackCoreS3SE,GPIO_NUM_36,GPIO_NUM_37, GPIO_NUM_35, 255        , 255       , GPIO_NUM_4  },
{ board_t::board_M5StackChan  , GPIO_NUM_36, GPIO_NUM_37, GPIO_NUM_35, 255        , 255       , GPIO_NUM_4  },
{ board_t::board_M5Capsule    , GPIO_NUM_14, GPIO_NUM_12, GPIO_NUM_39, 255        , 255       , GPIO_NUM_11 },
{ board_t::board_M5Cardputer  , GPIO_NUM_40, GPIO_NUM_14, GPIO_NUM_39, 255        , 255       , GPIO_NUM_12 },
{ board_t::board_M5CardputerADV,GPIO_NUM_40, GPIO_NUM_14, GPIO_NUM_39, 255        , 255       , GPIO_NUM_12 },
{ board_t::board_M5PaperS3    , GPIO_NUM_39, GPIO_NUM_38, GPIO_NUM_40, 255        , 255       , GPIO_NUM_47 },
{ board_t::board_M5PaperDIY   , GPIO_NUM_39, GPIO_NUM_38, GPIO_NUM_40, 255        , 255       , GPIO_NUM_47 },
{ board_t::board_M5StampPLC   , GPIO_NUM_7,  GPIO_NUM_8,  GPIO_NUM_9,  255        , 255       , GPIO_NUM_10 },
{ board_t::board_M5PaperColor , GPIO_NUM_15, GPIO_NUM_13, GPIO_NUM_14, 255        , 255       , GPIO_NUM_47 },
{ board_t::board_M5PaperMono  , GPIO_NUM_13, GPIO_NUM_12, GPIO_NUM_11, GPIO_NUM_10, GPIO_NUM_9, GPIO_NUM_8  },
#elif defined (CONFIG_IDF_TARGET_ESP32C3)
#elif defined (CONFIG_IDF_TARGET_ESP32C6)
#elif defined (CONFIG_IDF_TARGET_ESP32C61)
{ board_t::board_M5CoreMatrix , GPIO_NUM_25, GPIO_NUM_27, GPIO_NUM_26, 255        , 255        , GPIO_NUM_28 },
#elif defined (CONFIG_IDF_TARGET_ESP32H2)
#elif defined (CONFIG_IDF_TARGET_ESP32P4)
{ board_t::board_M5CoreP4X    , GPIO_NUM_10, GPIO_NUM_7 , GPIO_NUM_8 , 255        , 255        , GPIO_NUM_50 },
{ board_t::board_M5Tab5       , GPIO_NUM_43, GPIO_NUM_44, GPIO_NUM_39, GPIO_NUM_40, GPIO_NUM_41, GPIO_NUM_42 },
#elif defined (CONFIG_IDF_TARGET_ESP32C5)
{ board_t::board_M5ToughC5    , GPIO_NUM_9 , GPIO_NUM_7 , GPIO_NUM_8 , 255        , 255        , GPIO_NUM_10 },
#elif !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
{ board_t::board_M5Stack      , GPIO_NUM_18, GPIO_NUM_23, GPIO_NUM_19, 255        , 255        , GPIO_NUM_4  },
{ board_t::board_M5StackCore2 , GPIO_NUM_18, GPIO_NUM_23, GPIO_NUM_38, 255        , 255        , GPIO_NUM_4  },
{ board_t::board_M5Tough      , GPIO_NUM_18, GPIO_NUM_23, GPIO_NUM_38, 255        , 255        , GPIO_NUM_4  },
{ board_t::board_M5Paper      , GPIO_NUM_14, GPIO_NUM_12, GPIO_NUM_13, 255        , 255        , GPIO_NUM_4  },
#endif
{ board_t::board_unknown      , 255        , 255        , 255        , 255        , 255        , 255         },
};

static constexpr const uint8_t _pin_table_other0[][2] = {
                             //RGBLED
#if defined (CONFIG_IDF_TARGET_ESP32S3)
{ board_t::board_M5AtomS3U    , GPIO_NUM_35 },
{ board_t::board_M5AtomS3Lite , GPIO_NUM_35 },
{ board_t::board_M5StampS3    , GPIO_NUM_21 },
{ board_t::board_M5DualKey    , GPIO_NUM_21 },
{ board_t::board_M5StampPLC   , GPIO_NUM_21 },
{ board_t::board_M5AirQ       , GPIO_NUM_21 },
{ board_t::board_M5Dial       , GPIO_NUM_21 },
{ board_t::board_M5DinMeter   , GPIO_NUM_21 },
{ board_t::board_M5Capsule    , GPIO_NUM_21 },
{ board_t::board_M5Cardputer  , GPIO_NUM_21 },
{ board_t::board_M5CardputerADV,GPIO_NUM_21 },
{ board_t::board_M5PaperColor , GPIO_NUM_21 },
#elif defined (CONFIG_IDF_TARGET_ESP32C3)
{ board_t::board_M5StampC3    , GPIO_NUM_2  },
{ board_t::board_M5StampC3U   , GPIO_NUM_2  },
#elif defined (CONFIG_IDF_TARGET_ESP32C6)
{ board_t::board_M5NanoC6     , GPIO_NUM_20 },
{ board_t::board_M5UnitC6L    , GPIO_NUM_2  },
#elif defined (CONFIG_IDF_TARGET_ESP32C61)
#elif defined (CONFIG_IDF_TARGET_ESP32H2)
{ board_t::board_M5NanoH2     , GPIO_NUM_11 },
#elif defined (CONFIG_IDF_TARGET_ESP32C5)
#elif !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
{ board_t::board_M5Stack      , GPIO_NUM_15 },
{ board_t::board_M5StackCore2 , GPIO_NUM_25 },
{ board_t::board_M5Station    , GPIO_NUM_4  },
{ board_t::board_M5AtomLite   , GPIO_NUM_27 },
{ board_t::board_M5AtomMatrix , GPIO_NUM_27 },
{ board_t::board_M5AtomVoice  , GPIO_NUM_27 },
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
#elif defined (CONFIG_IDF_TARGET_ESP32C61)
#elif defined (CONFIG_IDF_TARGET_ESP32H2)
#elif defined (CONFIG_IDF_TARGET_ESP32C5)
#elif !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)

{ board_t::board_M5StickCPlus2 , GPIO_NUM_4  },
{ board_t::board_M5Paper       , GPIO_NUM_2  },
{ board_t::board_M5StackCoreInk, GPIO_NUM_12 },
{ board_t::board_M5TimerCam    , GPIO_NUM_33 },

#endif
{ board_t::board_unknown      , 255         },
};

static constexpr const uint8_t _pin_table_mbus[][31] = {
#if defined (CONFIG_IDF_TARGET_ESP32P4)
{ board_t::board_M5CoreP4X,
  255        , GPIO_NUM_17,
  255        , GPIO_NUM_20,
  255        , 255        ,
  GPIO_NUM_7 , GPIO_NUM_21,
  GPIO_NUM_8 , GPIO_NUM_22,
  GPIO_NUM_10, 255        ,
  GPIO_NUM_38, GPIO_NUM_37,
  GPIO_NUM_15, GPIO_NUM_14,
  GPIO_NUM_11, GPIO_NUM_9 ,
  GPIO_NUM_18, GPIO_NUM_16,
  GPIO_NUM_39, GPIO_NUM_12,
  GPIO_NUM_34, GPIO_NUM_23,
  255        , GPIO_NUM_19,
  255        , 255        ,
  255        , 255        ,
},
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
{ board_t::board_M5StackChan,
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
#elif defined (CONFIG_IDF_TARGET_ESP32C61)
{ board_t::board_M5CoreMatrix,
  255        , GPIO_NUM_3 ,
  255        , GPIO_NUM_4 ,
  255        , 255        ,
  GPIO_NUM_27, GPIO_NUM_5 ,
  GPIO_NUM_26, GPIO_NUM_6 ,
  GPIO_NUM_25, 255        ,
  GPIO_NUM_10, GPIO_NUM_11,
  GPIO_NUM_7 , GPIO_NUM_8 ,
  GPIO_NUM_0 , GPIO_NUM_1 ,
  GPIO_NUM_0 , GPIO_NUM_1 ,
  GPIO_NUM_23, GPIO_NUM_22,
  GPIO_NUM_24, GPIO_NUM_9 ,
  255        , GPIO_NUM_29,
  255        , 255        ,
  255        , 255        ,
},
#elif defined (CONFIG_IDF_TARGET_ESP32H2)
#elif defined (CONFIG_IDF_TARGET_ESP32C5)
#elif !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
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
{ board_t::board_M5Tough,
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
    if (id == board_t::board_M5Tab5X) {
      id = board_t::board_M5Tab5;
    }

    constexpr const std::pair<const void*, size_t> tbl[] = {
      { _pin_table_i2c_ex_in, sizeof(_pin_table_i2c_ex_in[0]) },
      { _pin_table_port_bc, sizeof(_pin_table_port_bc[0]) },
      { _pin_table_port_de, sizeof(_pin_table_port_de[0]) },
      { _pin_table_sd, sizeof(_pin_table_sd[0]) },
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

  /// @return true when every write in the table was acknowledged.
  static bool in_i2c_bulk_write(const uint8_t i2c_addr, const uint8_t* bulk_data, const uint32_t freq = 100000u, const uint8_t retry = 0)
  {
    // bulk_data example..
    // const uint8_t bulk_data[] = {
    //   2, 0x00, 0x00,       // <- datalen = 2, reg = 0x00, data = 0x00
    //   3, 0x01, 0x00, 0x02, // <- datalen = 3, reg = 0x01, data = 0x00, 0x02
    //   0 };                 // <- datalen 0 is end of data.

    bool all_ok = true;
    while (*bulk_data) {
      uint8_t len = *bulk_data++;
      uint8_t r = retry + 1;
      while (!M5.In_I2C.writeRegister(i2c_addr, bulk_data[0], &bulk_data[1], len - 1, freq) && --r) { m5gfx::delay(1); }
      all_ok &= (r != 0);
      bulk_data += len;
    }
    return all_ok;
  }

  static constexpr uint8_t es7210_i2c_addr = 0x40;
  static constexpr uint8_t es8311_i2c_addr0 = 0x18;
  static constexpr uint8_t es8311_i2c_addr1 = 0x19;

#if defined (CONFIG_IDF_TARGET_ESP32S3)
  /// The ES8311 arms its capture path only through a SYSTEM(0x0D) write done
  /// while the I2S clock is running; a pre-clock write is absorbed silently
  /// (it even reads back). The arming is needed once per codec power-on
  /// reset and survives I2C power-down/up cycles.
  static std::atomic<bool> es8311_capture_armed { false };

  /// Post-start callback: arms the capture path once the I2S clock runs.
  /// The analog stage then warms up on its own (~1 s after power-up); that
  /// is chip physics, so begin() pays only for the arming here.
  static bool _microphone_post_start_cb_stopwatch(void* args)
  {
    (void)args;
    if (es8311_capture_armed.load(std::memory_order_acquire)) { return true; }
    m5gfx::i2c::i2c_temporary_switcher_t backup_i2c_setting(1, GPIO_NUM_47, GPIO_NUM_48);
    bool ok = false;
    bool last_ok = false;
    for (int i = 0; i < 3; ++i)
    { // writes at ~+30/+60/+90 ms after the clock start; the earliest arming
      // observed on hardware is ~+25 ms, the later points are the margin
      m5gfx::delay(30);
      last_ok = M5.In_I2C.writeRegister8(es8311_i2c_addr0, 0x0D, 0x01, 100000);
      ok |= last_ok;
    }
    backup_i2c_setting.restore();
    /// an acknowledge proves only the transport, so the latch requires the
    /// latest (most conservative) write to have been acknowledged
    if (last_ok) { es8311_capture_armed.store(true, std::memory_order_release); }
    return ok;
  }
#endif
  static constexpr uint8_t es8388_i2c_addr = 0x10;
  static constexpr uint8_t pi4io1_i2c_addr = 0x43;
#if defined (CONFIG_IDF_TARGET_ESP32S3)
  static constexpr uint8_t aw88298_i2c_addr = 0x36;
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
      Power_Class::_core_s3_aw9523_bit(0x02, 0b00000100, true);
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
      Power_Class::_core_s3_aw9523_bit(0x02, 0b00000100, false);
    }
#endif
    return true;
  }

  bool M5Unified::_speaker_enabled_cb_sticks3(void* args, bool enabled)
  {
    (void)args;
    (void)enabled;
#if defined (CONFIG_IDF_TARGET_ESP32S3)
    auto self = (M5Unified*)args;
    auto spk_cfg = self->Speaker.config();
    if (spk_cfg.pin_bck == GPIO_NUM_17)
    {
      static constexpr const uint8_t enabled_bulk_data[] = {
        2, 0x00, 0x80,  // 0x00 RESET/  CSM POWER ON
        2, 0x01, 0xB5,  // 0x01 CLOCK_MANAGER/ MCLK=BCLK
        2, 0x02, 0x18,  // 0x02 CLOCK_MANAGER/ MULT_PRE=3
        2, 0x0D, 0x01,  // 0x0D SYSTEM/ Power up analog circuitry
        2, 0x12, 0x00,  // 0x12 SYSTEM/ power-up DAC - NOT default
        2, 0x13, 0x10,  // 0x13 SYSTEM/ Enable output to HP drive - NOT default
        2, 0x32, 0xBF,  // 0x32 DAC/ DAC volume (0xBF == ±0 dB )
        2, 0x37, 0x08,  // 0x37 DAC/ Bypass DAC equalizer - NOT default
        0
      };
      if (enabled)
      {
        self->In_I2C.bitOn(m5pm1_i2c_addr, 0x11, 0b00001000, 100000);
        ESP_LOGD("M5Unified", "enabling es8311\n");
        in_i2c_bulk_write(es8311_i2c_addr0, enabled_bulk_data, 100000, 3);
      }
      else /// disableにする場合および内蔵スピーカ以外を操作対象とした場合、内蔵スピーカを停止する。
      {
        ESP_LOGD("M5Unified", "disabling es8311\n");
        self->In_I2C.bitOff(m5pm1_i2c_addr, 0x11, 0b00001000, 100000);
      }
//*/
    }
#endif
    return true;
  }

  bool M5Unified::_speaker_enabled_cb_papercolor(void* args, bool enabled)
  {
    (void)args;
    (void)enabled;
#if defined (CONFIG_IDF_TARGET_ESP32S3)
    auto self = (M5Unified*)args;
    auto spk_cfg = self->Speaker.config();
    gpio_num_t codec_en_pin = GPIO_NUM_45;
    gpio_num_t spk_en_pin = GPIO_NUM_46;
    m5gfx::pinMode(codec_en_pin, m5gfx::pin_mode_t::output);
    m5gfx::pinMode(spk_en_pin, m5gfx::pin_mode_t::output);
    if (spk_cfg.pin_bck == GPIO_NUM_40)
    {
      static constexpr const uint8_t enabled_bulk_data[] = {
        2, 0x00, 0x80,  // 0x00 RESET/  CSM POWER ON
        2, 0x01, 0xB5,  // 0x01 CLOCK_MANAGER/ MCLK=BCLK
        2, 0x02, 0x18,  // 0x02 CLOCK_MANAGER/ MULT_PRE=3
        2, 0x0D, 0x01,  // 0x0D SYSTEM/ Power up analog circuitry
        2, 0x12, 0x00,  // 0x12 SYSTEM/ power-up DAC - NOT default
        2, 0x13, 0x10,  // 0x13 SYSTEM/ Enable output to HP drive - NOT default
        2, 0x32, 0xCF,  // 0x32 DAC/ DAC volume (0xCF == +16 dB )
        2, 0x37, 0x08,  // 0x37 DAC/ Bypass DAC equalizer - NOT default
        0
      };
      if (enabled)
      {
        m5gfx::gpio_hi(codec_en_pin);
        m5gfx::gpio_hi(spk_en_pin);
        in_i2c_bulk_write(es8311_i2c_addr0, enabled_bulk_data, 100000, 3);
      }
      else
      {
        m5gfx::gpio_lo(codec_en_pin);
        m5gfx::gpio_lo(spk_en_pin);
      }
    }
#endif
    return true;
  }

  bool M5Unified::_speaker_enabled_cb_stopwatch(void* args, bool enabled)
  {
#if defined (CONFIG_IDF_TARGET_ESP32S3)
    auto self = (M5Unified*)args;

    static constexpr const uint8_t enabled_bulk_data[] = {
      2, 0x00, 0x80,  // 0x00 RESET/  CSM POWER ON
      2, 0x01, 0xB5,  // 0x01 CLOCK_MANAGER/ MCLK=BCLK
      2, 0x02, 0x18,  // 0x02 CLOCK_MANAGER/ MULT_PRE=3
      2, 0x0D, 0x01,  // 0x0D SYSTEM/ Power up analog circuitry
      2, 0x12, 0x00,  // 0x12 SYSTEM/ power-up DAC - NOT default
      2, 0x13, 0x10,  // 0x13 SYSTEM/ Enable output to HP drive - NOT default
      2, 0x32, 0xCB,  // 0x32 DAC/ DAC volume +6 dB (0xBF == 0 dB, 0.5 dB/step). Reaches full-scale at max master volume; higher values clip digitally without adding loudness
      2, 0x37, 0x08,  // 0x37 DAC/ Bypass DAC equalizer - NOT default
      0
    };
    if (enabled)
    {
      auto& ioe1 = self->getIOExpander(0);
      ioe1.digitalWrite(M5IOE1_Class::gpio3, true); // Enable Audio Power (M5IOE1_G3)
      self->delay(10);
      in_i2c_bulk_write(es8311_i2c_addr0, enabled_bulk_data, 100000, 3);
      ioe1.digitalWrite(M5IOE1_Class::gpio10, true); // Enable PA (M5IOE1_G10)
    }
    else
    { /// Keep Audio Power (M5IOE1_G3) on: cutting it forces another codec
      /// reset, re-arming and re-warm-up on the next capture (and a quickly
      /// cycled rail misfires). Only the DAC is powered down; Mic disable
      /// does the deeper I2C power-down.
      auto& ioe1 = self->getIOExpander(0);
      ioe1.digitalWrite(M5IOE1_Class::gpio10, false); // Disable PA (M5IOE1_G10)
      static constexpr const uint8_t disabled_bulk_data[] = {
        2, 0x12, 0x02,  // 0x12 SYSTEM/ power-down DAC
        0
      };
      in_i2c_bulk_write(es8311_i2c_addr0, disabled_bulk_data, 100000, 3);
    }
#endif
    return true;
  }

  bool M5Unified::_speaker_enabled_cb_chain_captain(void* args, bool enabled)
  {
#if defined (CONFIG_IDF_TARGET_ESP32S3)
    auto self = (M5Unified*)args;
    static constexpr const uint8_t enabled_bulk_data[] = {
      2, 0x00, 0x80,  // 0x00 RESET/  CSM POWER ON
      2, 0x01, 0xB5,  // 0x01 CLOCK_MANAGER/ MCLK=BCLK
      2, 0x02, 0x18,  // 0x02 CLOCK_MANAGER/ MULT_PRE=3
      2, 0x0D, 0x01,  // 0x0D SYSTEM/ Power up analog circuitry
      2, 0x12, 0x00,  // 0x12 SYSTEM/ power-up DAC - NOT default
      2, 0x13, 0x10,  // 0x13 SYSTEM/ Enable output to HP drive - NOT default
      2, 0x32, 0xEF,  // 0x32 DAC/ DAC volume (0xBF == ±0 dB )
      2, 0x37, 0x08,  // 0x37 DAC/ Bypass DAC equalizer - NOT default
      0
    };
    if (enabled)
    {
      self->getIOExpander(0).digitalWrite(M5IOE1_Class::gpio5, true); // M5IOE1_G5 audio rail
      self->delay(10);
      in_i2c_bulk_write(es8311_i2c_addr0, enabled_bulk_data, 100000, 3);
      m5gfx::gpio_hi(GPIO_NUM_21); // AW8737A one-wire enable, default mode
    }
    else
    {
      m5gfx::gpio_lo(GPIO_NUM_21);
      self->getIOExpander(0).digitalWrite(M5IOE1_Class::gpio5, false);
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
    if (enabled)
    {
      in_i2c_bulk_write(es8388_i2c_addr, enabled_bulk_data);
      // AMP on
      M5.In_I2C.bitOn(pi4io1_i2c_addr, 0x05, 0b00000010, 400000);
    }
    else
    { // 正規の power-down シーケンス。end() は cb(false) を I2S 停止より先に呼ぶため、
      // ここは MCLK/BCLK が生きているうちに実行される。旧実装 (reg8 のみ書き込み) では
      // DAC 稼働状態のままクロックが絶たれ、次回 enable の reset が毎回異なる残留状態
      // から行われて受信位相が begin ごとに不定になっていた。毎回同一の power-down
      // 状態へ落としてから終了することで、次回 enable を常に定義済み状態から始める。
      M5.In_I2C.bitOff(pi4io1_i2c_addr, 0x05, 0b00000010, 400000); // AMP off (過渡音を出さない)
      M5.In_I2C.writeRegister8(es8388_i2c_addr, 25, 0x24, 400000); // DACCONTROL3: mute (SoftRamp 維持)
      m5gfx::delay(1);                                             // soft-ramp 遷移待ち
      M5.In_I2C.writeRegister8(es8388_i2c_addr,  4, 0xC0, 400000); // DACPOWER: DAC L/R down + 全出力 off
      M5.In_I2C.writeRegister8(es8388_i2c_addr,  2, 0xFF, 400000); // CHIPPOWER: 全停止 (ADF deinit と同一の終端状態)
    }
#endif
    return true;
  }

#if defined (CONFIG_IDF_TARGET_ESP32P4)
  // Callers are P4-only as well; keep the definition inside the guard to avoid an unused-function warning elsewhere.
  static void _corep4x_audio_power(M5Unified* self, bool enabled)
  {
    // Speaker and microphone share M5IOE1_G1, so keep the rail enabled at runtime.
    if (!enabled) { return; }
    auto& ioe1 = self->getIOExpander(0);
    ioe1.setHighImpedance(M5IOE1_Class::gpio1, false);
    ioe1.setDirection(M5IOE1_Class::gpio1, true);
    ioe1.digitalWrite(M5IOE1_Class::gpio1, true);
    self->delay(20);
  }
#endif

  bool M5Unified::_speaker_enabled_cb_corep4x(void* args, bool enabled)
  {
#if defined (CONFIG_IDF_TARGET_ESP32P4)
    auto self = (M5Unified*)args;
    static constexpr const uint8_t enabled_bulk_data[] = {
      // ES8311 slave, 24 kHz, 16-bit I2S, MCLK = 256 * sample rate.
      2, 0x0D, 0xFA,
      2, 0x44, 0x08,
      2, 0x44, 0x08,
      2, 0x01, 0x30,
      2, 0x02, 0x00,
      2, 0x03, 0x10,
      2, 0x16, 0x24,
      2, 0x04, 0x10,
      2, 0x05, 0x00,
      2, 0x0B, 0x00,
      2, 0x0C, 0x00,
      2, 0x10, 0x1F,
      2, 0x11, 0x7F,
      2, 0x00, 0x80,
      2, 0x01, 0x3F,
      2, 0x06, 0x03,
      2, 0x13, 0x10,
      2, 0x1B, 0x0A,
      2, 0x1C, 0x6A,
      2, 0x44, 0x58,
      2, 0x09, 0x00,
      2, 0x17, 0xBF,
      2, 0x0E, 0x02,
      2, 0x12, 0x00,
      2, 0x14, 0x1A,
      2, 0x0D, 0x01,
      2, 0x15, 0x40,
      2, 0x37, 0x08,
      2, 0x45, 0x00,
      2, 0x07, 0x00,
      2, 0x08, 0xFF,
      2, 0x32, 0xBF,
      0
    };
    _corep4x_audio_power(self, enabled);
    auto& ioe1 = self->getIOExpander(0);
    ioe1.setHighImpedance(M5IOE1_Class::gpio3, false);
    ioe1.setDirection(M5IOE1_Class::gpio3, true);
    ioe1.digitalWrite(M5IOE1_Class::gpio3, enabled);
    if (enabled)
    {
      in_i2c_bulk_write(es8311_i2c_addr0, enabled_bulk_data, 100000, 3);
    }
#else
    (void)args;
    (void)enabled;
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

  bool M5Unified::_speaker_enabled_cb_cardputer_adv(void* args, bool enabled)
  {
    (void)args;
    (void)enabled;
#if defined (CONFIG_IDF_TARGET_ESP32S3)
    static constexpr const uint8_t enabled_bulk_data[] = {
      2, 0x00, 0x80,  // 0x00 RESET/  CSM POWER ON
      2, 0x01, 0xB5,  // 0x01 CLOCK_MANAGER/ MCLK=BCLK
      2, 0x02, 0x18,  // 0x02 CLOCK_MANAGER/ MULT_PRE=3
      2, 0x0D, 0x01,  // 0x0D SYSTEM/ Power up analog circuitry
      2, 0x12, 0x00,  // 0x12 SYSTEM/ power-up DAC - NOT default
      2, 0x13, 0x10,  // 0x13 SYSTEM/ Enable output to HP drive - NOT default
      2, 0x32, 0xBF,  // 0x32 DAC/ DAC volume (0xBF == ±0 dB )
      2, 0x37, 0x08,  // 0x37 DAC/ Bypass DAC equalizer - NOT default
      0
    };
    static constexpr const uint8_t disabled_bulk_data[] = {
      0
    };

    in_i2c_bulk_write(es8311_i2c_addr0, enabled ? enabled_bulk_data : disabled_bulk_data);
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

  bool M5Unified::_microphone_enabled_cb_atom_echos3r(void* args, bool enabled)
  {
    (void)args;
    (void)enabled;
#if defined (CONFIG_IDF_TARGET_ESP32S3)
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
    m5gfx::i2c::i2c_temporary_switcher_t backup_i2c_setting(1, GPIO_NUM_45, GPIO_NUM_0);
    in_i2c_bulk_write(es8311_i2c_addr0, enabled ? enabled_bulk_data : disabled_bulk_data);
    backup_i2c_setting.restore();
#endif
    return true;
  }

  bool M5Unified::_speaker_enabled_cb_atom_echos3r(void* args, bool enabled)
  {
    (void)args;
    (void)enabled;
#if defined (CONFIG_IDF_TARGET_ESP32S3)
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

    m5gfx::i2c::i2c_temporary_switcher_t backup_i2c_setting(1, GPIO_NUM_45, GPIO_NUM_0);
    in_i2c_bulk_write(es8311_i2c_addr0, enabled ? enabled_bulk_data : disabled_bulk_data);
    gpio_num_t pin_en = GPIO_NUM_18;
    if (enabled)
    {
      m5gfx::pinMode(pin_en, m5gfx::pin_mode_t::output);
      m5gfx::gpio_hi(pin_en);
    }
    else
    { m5gfx::gpio_lo(pin_en); }
    backup_i2c_setting.restore();
#endif
    return true;
  }

  bool M5Unified::_microphone_enabled_cb_sticks3(void* args, bool enabled)
  {
    (void)args;
    (void)enabled;
#if defined (CONFIG_IDF_TARGET_ESP32S3)
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
    m5gfx::i2c::i2c_temporary_switcher_t backup_i2c_setting(1, GPIO_NUM_47, GPIO_NUM_48); // sda, scl
    in_i2c_bulk_write(es8311_i2c_addr0, enabled ? enabled_bulk_data : disabled_bulk_data);
    backup_i2c_setting.restore();
#endif
    return true;
  }

  bool M5Unified::_microphone_enabled_cb_papercolor(void* args, bool enabled)
  {
    (void)args;
    (void)enabled;
#if defined (CONFIG_IDF_TARGET_ESP32S3)
    auto self = (M5Unified*)args;
    auto cfg = self->Mic.config();
    gpio_num_t codec_en_pin = GPIO_NUM_45;
    m5gfx::pinMode(codec_en_pin, m5gfx::pin_mode_t::output);
    m5gfx::gpio_hi(codec_en_pin);
    delay(50);
    if (cfg.pin_bck == GPIO_NUM_40)
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
          { 0x44, 0x00 }, // MIC2_GAIN
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

  bool M5Unified::_microphone_enabled_cb_papermono(void* args, bool enabled)
  {
#if defined (CONFIG_IDF_TARGET_ESP32S3)
    auto self = (M5Unified*)args;
    auto& ioe1 = self->getIOExpander(0);
    bool result = true;
    if (enabled)
    {
      // The factory firmware enables PM1 BOOST before powering the PDM mic.
      result = self->Power.M5pm1.setExtOutput(true);
    }
    // M5IOE1 GPIO12 is the Paper Mono PDM microphone power enable.
    ioe1.setHighImpedance(M5IOE1_Class::gpio12, false);
    ioe1.setDirection(M5IOE1_Class::gpio12, true);
    ioe1.digitalWrite(M5IOE1_Class::gpio12, enabled);
    return result;
#else
    (void)args;
    (void)enabled;
#endif
    return true;
  }


  bool M5Unified::_microphone_enabled_cb_stopwatch(void* args, bool enabled)
  {
#if defined (CONFIG_IDF_TARGET_ESP32S3)
    auto self = (M5Unified*)args;

    static constexpr const uint8_t enabled_bulk_data[] = {
      2, 0x00, 0x80,  // 0x00 RESET/  CSM POWER ON
      2, 0x01, 0xBA,  // 0x01 CLOCK_MANAGER/ MCLK=BCLK
      2, 0x02, 0x18,  // 0x02 CLOCK_MANAGER/ MULT_PRE=3
      2, 0x0D, 0x01,  // 0x0D SYSTEM/ Power up analog circuitry
      2, 0x0E, 0x02,  // 0x0E SYSTEM/ : Enable analog PGA, enable ADC modulator
      2, 0x14, 0x17,  // ES8311_ADC_REG14 : select Mic1p-Mic1n / analog PGA +21 dB (gain code 7, 3 dB/code; SNR plateaus by this level)
      2, 0x17, 0xCB,  // ES8311_ADC_REG17 : ADC_VOLUME +6 dB (0xBF == 0 dB, 0.5 dB/step)
      2, 0x1C, 0x64,  // ES8311_ADC_REG1C : ADC EQ bypass, dynamic HPF, HPF stage-2 coeff 4 (cuts sub-200 Hz rumble at the default 16 kHz rate)
      0
    };
    static constexpr const uint8_t disabled_bulk_data[] = {
      2, 0x0D, 0xFC,  // 0x0D SYSTEM/ Power down analog circuitry
      2, 0x0E, 0x6A,  // 0x0E SYSTEM
      2, 0x00, 0x00,  // 0x00 RESET/  CSM POWER DOWN
      0
    };
    if (enabled)
    {
      self->getIOExpander(0).digitalWrite(M5IOE1_Class::gpio3, true); // Enable Audio Power (M5IOE1_G3)
      self->delay(5);
    }
    m5gfx::i2c::i2c_temporary_switcher_t backup_i2c_setting(1, GPIO_NUM_47, GPIO_NUM_48);
    if (enabled)
    { /// 0x17 loses the value a previous setup wrote on any codec reset,
      /// so it witnesses a reset done outside this library: re-arm then.
      uint8_t v = 0;
      if (!M5.In_I2C.readRegister(es8311_i2c_addr0, 0x17, &v, 1, 100000) || v != 0xCB)
      {
        es8311_capture_armed.store(false, std::memory_order_release);
      }
    }
    bool setup_ok = in_i2c_bulk_write(es8311_i2c_addr0, enabled ? enabled_bulk_data : disabled_bulk_data, 100000, 3);
    backup_i2c_setting.restore();
    /// a codec with an incomplete setup must not be published as working
    if (enabled && !setup_ok) { return false; }
#endif
    return true;
  }

  bool M5Unified::_microphone_enabled_cb_chain_captain(void* args, bool enabled)
  {
#if defined (CONFIG_IDF_TARGET_ESP32S3)
    auto self = (M5Unified*)args;
    static constexpr const uint8_t enabled_bulk_data[] = {
      2, 0x00, 0x80,  // RESET / CSM power on
      2, 0x01, 0xBA,  // MCLK from BCLK
      2, 0x02, 0x18,  // clock multiplier
      2, 0x0D, 0x01,  // power up analog circuitry
      2, 0x0E, 0x02,  // enable analog PGA and ADC modulator
      2, 0x14, 0x10,  // differential microphone input, minimum PGA gain
      2, 0x17, 0xFF,  // ADC volume
      2, 0x1C, 0x6A,  // bypass ADC equalizer and cancel DC offset
      0
    };
    static constexpr const uint8_t disabled_bulk_data[] = {
      2, 0x0D, 0xFC,
      2, 0x0E, 0x6A,
      2, 0x00, 0x00,
      0
    };
    if (enabled)
    {
      self->getIOExpander(0).digitalWrite(M5IOE1_Class::gpio5, true); // M5IOE1_G5 audio rail
      self->delay(5);
    }
    in_i2c_bulk_write(es8311_i2c_addr0, enabled ? enabled_bulk_data : disabled_bulk_data, 100000, 3);
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

  bool M5Unified::_microphone_enabled_cb_corep4x(void* args, bool enabled)
  {
#if defined (CONFIG_IDF_TARGET_ESP32P4)
    auto self = (M5Unified*)args;
    _corep4x_audio_power(self, enabled);
    self->In_I2C.writeRegister8(es7210_i2c_addr, 0x00, 0xFF, 400000);
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
        2, 0x45, 0x1B, // MIC3_GAIN (AEC input)
        2, 0x46, 0x1B, // MIC4_GAIN (TDM slot 4)
        2, 0x47, 0x00, // MIC1_LP
        2, 0x48, 0x00, // MIC2_LP
        2, 0x49, 0x00, // MIC3_LP
        2, 0x4A, 0x00, // MIC4_LP
        2, 0x4B, 0x00, // MIC12_PDN
        2, 0x4C, 0x00, // MIC34_PDN
        2, 0x01, 0x14, // CLK_ON_OFF
        0,
      };
      in_i2c_bulk_write(es7210_i2c_addr, data, 100000, 3);
    }
#else
    (void)args;
    (void)enabled;
#endif
    return true;
  }

  bool M5Unified::_microphone_enabled_cb_cardputer_adv(void* args, bool enabled)
  {
    (void)args;
    (void)enabled;
#if defined (CONFIG_IDF_TARGET_ESP32S3)
    static constexpr const uint8_t enabled_bulk_data[] = {
      2, 0x00, 0x80,  // 0x00 RESET/  CSM POWER ON
      2, 0x01, 0xBA,  // 0x01 CLOCK_MANAGER/ MCLK=BCLK
      2, 0x02, 0x18,  // 0x02 CLOCK_MANAGER/ MULT_PRE=3
      2, 0x0D, 0x01,  // 0x0D SYSTEM/ Power up analog circuitry
      2, 0x0E, 0x02,  // 0x0E SYSTEM/ : Enable analog PGA, enable ADC modulator
      2, 0x14, 0x10,  // ES8311_ADC_REG14 : select Mic1p-Mic1n / PGA GAIN (minimum)
      2, 0x17, 0xBF,  // ES8311_ADC_REG17 : ADC_VOLUME 0xBF == ± 0 dB
      2, 0x1C, 0x6A,  // ES8311_ADC_REG1C : ADC Equalizer bypass, cancel DC offset in digital domain
      0
    };
    static constexpr const uint8_t disabled_bulk_data[] = {
      2, 0x0D, 0xFC,  // 0x0D SYSTEM/ Power down analog circuitry
      2, 0x0E, 0x6A,  // 0x0E SYSTEM
      2, 0x00, 0x00,  // 0x00 RESET/  CSM POWER DOWN
      0
    };

    in_i2c_bulk_write(es8311_i2c_addr0, enabled ? enabled_bulk_data : disabled_bulk_data);
#endif
    return true;
  }

#if defined (M5UNIFIED_PC_BUILD)
#elif !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
  static constexpr gpio_num_t TFCARD_CS_PIN          = GPIO_NUM_4;
  static constexpr gpio_num_t CoreInk_BUTTON_EXT_PIN = GPIO_NUM_5;
  static constexpr gpio_num_t CoreInk_BUTTON_PWR_PIN = GPIO_NUM_27;
#endif

  /// probe を始める前に一度だけ、デバイスの電源が安定するのを待つ。
  /// 待ちが要るのは「電源投入から間もない」ことであってアドレスごとの事情ではないので、
  /// 判定の入口で一度払う。旧実装は最初の probe が常に真を返して連鎖が止まっていたため
  /// 結果的に 1 回しか待っていなかった。それを意図として書き直したもの。
  void M5Unified::_wait_i2c_device_power(void)
  {
#if !defined(M5UNIFIED_PC_BUILD)
    static bool waited = false;
    if (!waited)
    {
      waited = true;
      m5gfx::delay(50);
    }
#endif
  }

  bool M5Unified::_probe_i2c_addr(uint8_t sda, uint8_t scl, uint8_t addr)
  {
#if defined(M5UNIFIED_PC_BUILD)
    return false;
#else
    /// アドレスの存在確認はソフトウェア I2C ポートで行う (オープンドレイン駆動で
    /// ACK 競合が起きず、ハードウェアのペリフェラルにも触れない)。
    /// ポートは M5GFX の autodetect probe (-1) と分ける。ソフト I2C のスロットは
    /// 所有権を持たず、init が既存の設定を黙って奪う作りなので、ライブラリ同士で
    /// 同じスロットを共有しない。
    static constexpr int_fast16_t probe_i2c_port = -2;

    _wait_i2c_device_power();

    m5gfx::gpio::pin_backup_t pin_backup[] = { scl, sda };

    // ここでは待たない。電源安定待ちは判定の入口で一度だけ行う (_wait_i2c_device_power)。
    // アドレスごとに待つと、判定が空振りするたびに数十 ms が起動時間へ積み上がる。
    m5gfx::pinMode(scl, m5gfx::pin_mode_t::input_pullup);
    m5gfx::pinMode(sda, m5gfx::pin_mode_t::input_pullup);

    // 指定ピンが「外部プルアップの載った I2C バス」かどうかを先に確かめる。
    // ここは I2C ピンとは限らない場所を駆動する機種判別なので、判定を外すと
    // 機種の読みが変わる。判定方式は M5GFX の autodetect probe と同一のものを使う
    // (実機での実績がある形から動かさない。変えるなら該当機種すべてで再検証が要る)。
    //
    // 4 回の read のうち、後半 2 回は入力プルダウンにしても High になること
    // (= 外部プルアップが内部プルダウンに勝つこと) を確認するもの。
    // 弱いプルアップ (内部プルダウンと同程度の抵抗値) では通らないが、
    // 「強いプルアップがある = I2C バスである」を要求するのがこの判定の趣旨。
    const uint8_t cmd_bus_check_list[] = {
      m5gfx::gpio::command_write_low          , scl,
      m5gfx::gpio::command_read               , scl,  // low チェック
      m5gfx::gpio::command_write_low          , sda,
      m5gfx::gpio::command_read               , sda,  // low チェック
      m5gfx::gpio::command_mode_input_pulldown, scl,
      m5gfx::gpio::command_delay_usec         , 10,
      m5gfx::gpio::command_read               , scl,  // 外部プルアップがあるなら High
      m5gfx::gpio::command_mode_input_pullup  , scl,
      m5gfx::gpio::command_mode_input_pulldown, sda,
      m5gfx::gpio::command_delay_usec         , 10,
      m5gfx::gpio::command_read               , sda,  // 外部プルアップがあるなら High
      m5gfx::gpio::command_mode_input_pullup  , sda,
      m5gfx::gpio::command_end
    };
    auto bus_check = [&](void) -> uint32_t
    {
      // ラッチを Low にしてから出力へ切り替える。直前は input_pullup (ラッチ High)
      // なので、先に出力にすると一瞬 push-pull で High を駆動してしまう。
      // ここは相手が何か分からないピンなので、High は一度も駆動しない。
      m5gfx::gpio_lo(scl); m5gfx::pinMode(scl, m5gfx::pin_mode_t::output);
      m5gfx::gpio_lo(sda); m5gfx::pinMode(sda, m5gfx::pin_mode_t::output);
      return m5gfx::gpio::command(cmd_bus_check_list);
    };

    // 0x02 は「SCL は外部プルアップで戻るのに SDA だけ Low のまま」。前回の通信の
    // 途中で止まったデバイスがデータ線を握っている典型で (ソフトリセットでは
    // デバイスの電源が切れないため実際に起きる)、プルアップの無いただの Low ピンとは
    // このシグネチャで区別できる。この場合だけは STOP を見せて握りを解かせ、
    // もう一度確かめる。それ以外の不一致は I2C バスではないとみなして即座に帰る。
    uint32_t check = bus_check();
    if (check != 0x03 && check != 0x02)
    {
      for (auto& backup : pin_backup) { backup.restore(); }
      return false;
    }

    // 前回の通信の途中で止まっているデバイスに STOP を見せて論理状態を戻す。
    // START だけで戻らないデバイスが実在する (同ファイルの StampS3/Capsule 判別に
    // 「STOP を出さないと正しく動作しないデバイスがあった (UnitHEART MAX30100)」の記録がある)。
    // 線を Low へ駆動するか解放するかの 2 状態しか使わない (High を駆動しない) ので、
    // デバイスが線を握っていてもパッド同士の衝突にはならない。
    {
      auto line_lo = [](uint8_t pin)
      { m5gfx::gpio_lo(pin); m5gfx::pinMode(pin, m5gfx::pin_mode_t::output); };
      auto line_hi = [](uint8_t pin)
      { m5gfx::pinMode(pin, m5gfx::pin_mode_t::input_pullup); };
      for (int i = 0; i < 8; ++i)
      {
        line_lo(scl); m5gfx::delayMicroseconds(5);
        line_lo(sda); m5gfx::delayMicroseconds(5);
        line_hi(scl); m5gfx::delayMicroseconds(5);
        line_hi(sda); m5gfx::delayMicroseconds(5);  // SCL High 中の SDA Low->High = STOP
      }
    }

    if (check != 0x03 && bus_check() != 0x03)
    { // 握りが解けなかった。ここから先は probe しても意味がない。
      for (auto& backup : pin_backup) { backup.restore(); }
      return false;
    }

    bool hit = false;
    if (m5gfx::i2c::init(probe_i2c_port, sda, scl).has_value())
    {
      // クロックが上がらないバス、前の通信の途中でデバイスがデータ線を握っている
      // バスは、いずれも beginTransaction 側が検出して復旧または中断する。
      // NACK の場合も beginTransaction 自体は成功を返し (内部で STOP を出して
      // エラーをラッチする)、そのエラーは endTransaction が報告する。
      // したがって両方の成功をもって「ACK が返った」と判定する。
      hit = m5gfx::i2c::beginTransaction(probe_i2c_port, addr, 100000, false).has_value()
         && m5gfx::i2c::endTransaction(probe_i2c_port).has_value();
      m5gfx::i2c::release(probe_i2c_port);
    }
    for (auto& backup : pin_backup) {
        backup.restore();
    }
    return hit;
#endif
  }

  board_t M5Unified::_check_boardtype(board_t board)
  {
    return board;
  }

  board_t M5Unified::_default_fallback_board(void)
  {
    // Build selection follows explicit fallback and detector candidates.
#if defined (M5UNIFIED_PC_BUILD)
    return board_t::board_unknown;
#elif !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
#if defined (ARDUINO_M5STACK_CORE_ESP32) || defined (ARDUINO_M5STACK_FIRE) || defined (ARDUINO_M5Stack_Core_ESP32)
    return board_t::board_M5Stack;
#elif defined (ARDUINO_M5STACK_CORE2) || defined (ARDUINO_M5STACK_Core2)
    return board_t::board_M5StackCore2;
#elif defined (ARDUINO_M5STICK_C) || defined (ARDUINO_M5Stick_C)
    return board_t::board_M5StickC;
#elif defined (ARDUINO_M5STICK_C_PLUS) || defined (ARDUINO_M5Stick_C_Plus)
    return board_t::board_M5StickCPlus;
#elif defined (ARDUINO_M5STACK_COREINK) || defined (ARDUINO_M5Stack_CoreInk)
    return board_t::board_M5StackCoreInk;
#elif defined (ARDUINO_M5STACK_PAPER) || defined (ARDUINO_M5STACK_Paper)
    return board_t::board_M5Paper;
#elif defined (ARDUINO_M5STACK_TOUGH)
    return board_t::board_M5Tough;
#elif defined (ARDUINO_M5STACK_ATOM) || defined (ARDUINO_M5Stack_ATOM)
    return board_t::board_M5AtomLite;
#elif defined (ARDUINO_M5STACK_TIMER_CAM) || defined (ARDUINO_M5Stack_Timer_CAM)
    return board_t::board_M5TimerCam;
#endif
    // Pure exclusion belongs to package defaults, below build selection.
    const auto pkg = m5gfx::get_pkg_ver();
    if (pkg == 6) { return board_t::board_M5AtomPsram; } // PICO-V3
    if (pkg == 5) { return board_t::board_M5StampPico; } // PICO-D4
    // The legacy detector defaulted D0WDQ6 to TimerCam after other probes failed.
    if (pkg == EFUSE_RD_CHIP_VER_PKG_ESP32D0WDQ6) { return board_t::board_M5TimerCam; }
    return board_t::board_M5AtomLite;
#elif defined (CONFIG_IDF_TARGET_ESP32S3)
#if defined (BOARD_ID) && BOARD_ID == 147
    return board_t::board_M5DualKey;
#endif
    // ESP32-S3 package 0 is QFN56 and package 1 is LGA56.
    if (m5gfx::get_pkg_ver() == 1) { return board_t::board_M5StampS3Mini; }
    return board_t::board_M5StampS3;
#elif defined (CONFIG_IDF_TARGET_ESP32C3)
    return board_t::board_M5StampC3U;
#elif defined (CONFIG_IDF_TARGET_ESP32C6)
    // Preserve legacy package/flash defaults when no detector confirms a board.
    if (m5gfx::get_pkg_ver() == 1)
    {
      return REG_GET_FIELD(EFUSE_RD_MAC_SPI_SYS_4_REG, EFUSE_FLASH_CAP) == 2
           ? board_t::board_M5StampC6 : board_t::board_M5NanoC6;
    }
    return board_t::board_unknown;
#elif defined (CONFIG_IDF_TARGET_ESP32H2)
    return board_t::board_M5NanoH2;
#elif defined (CONFIG_IDF_TARGET_ESP32C5)
    return board_t::board_M5StampC5;
#elif defined (CONFIG_IDF_TARGET_ESP32P4)
    // Unidentified P4 modules have no display or board-specific power contract.
    esp_chip_info_t info;
    esp_chip_info(&info);
    return info.revision >= 300 ? board_t::board_M5StampP4X : board_t::board_M5StampP4;
#else
    return board_t::board_unknown;
#endif
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
#if SOC_I2C_NUM == 1 || defined (CONFIG_IDF_TARGET_ESP32C6) || defined (CONFIG_IDF_TARGET_ESP32C5)
    i2c_port_t in_port = I2C_NUM_0;
// M5GFX が LP_I2C 対応をコンパイルする条件と同一に保つこと
// (条件を満たさない SDK では LP ポートを開けないため HP のまま運用する)
#if defined (CONFIG_IDF_TARGET_ESP32C5) && defined ( SOC_LP_I2C_NUM ) && ( SOC_LP_I2C_NUM > 0 ) \
 && __has_include ( <driver/i2c_master.h> ) && defined ( ESP_IDF_VERSION_VAL ) && ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 4, 0)
    if (board == board_t::board_M5ToughC5)
    { /// 内部バス (G2/G3) は LP_I2C の固定パッドと一致するため LP ポートへ割り当てる。
      /// PortA も同じバスの物理分配なので Ex_I2C は同ポートを共有し (初代 BASIC と
      /// 同じ形)、HP の I2C0 は丸ごと空く。
      in_port = LP_I2C_NUM_0;
      ex_port = LP_I2C_NUM_0;
    }
#endif
#else
    i2c_port_t in_port = I2C_NUM_1;
    if (in_scl == ex_scl && in_sda == ex_sda) {
      in_port = ex_port;
    }
#endif
    if ((uint_fast8_t)in_scl < GPIO_NUM_MAX)
    {
      In_I2C.begin(in_port, in_sda, in_scl);
    }
    else
    {
      In_I2C.setPort(I2C_NUM_MAX, in_sda, in_scl);
    }

    if ((uint_fast8_t)ex_scl < GPIO_NUM_MAX)
    {
      if ((in_port != ex_port) || (in_sda == ex_sda && in_scl == ex_scl) || ((uint_fast8_t)in_scl >= GPIO_NUM_MAX)) {
        Ex_I2C.setPort(ex_port, ex_sda, ex_scl);
      }
    }

    switch (board) {
#if defined (CONFIG_IDF_TARGET_ESP32P4)
    case board_t::board_M5CoreP4X:
      {
        auto ioexp = new M5IOE1_Class(0x4F);
        ioexp->begin();
        _io_expander[0].reset(ioexp);
      }
      break;
    case board_t::board_M5Tab5:
    case board_t::board_M5Tab5X:
      for (int i = 0; i < 2; ++i)
      {
        auto ioexp = new PI4IOE5V6408_Class(0x43 + i);
        ioexp->begin();
        _io_expander[i].reset(ioexp);
      }
      break;
#elif defined (CONFIG_IDF_TARGET_ESP32C6)
    case board_t::board_M5UnitC6L:
      {
        auto ioexp = new PI4IOE5V6408_Class(0x43);
        ioexp->begin();
        _io_expander[0].reset(ioexp);
      }
      break;
    case board_t::board_ArduinoNessoN1:
      for (int i = 0; i < 2; ++i)
      {
        auto ioexp = new PI4IOE5V6408_Class(0x43 + i);
        ioexp->begin();
        _io_expander[i].reset(ioexp);
      }
      break;
#elif defined (CONFIG_IDF_TARGET_ESP32S3)
    case board_t::board_M5StampPLC:
      {
        auto ioexp = new PI4IOE5V6408_Class;
        ioexp->begin();
        _io_expander[0].reset(ioexp);
      }
      break;
    case board_t::board_M5ChainCaptain:
    case board_t::board_M5PaperMono:
    case board_t::board_M5StopWatch:
      {
        auto ioexp = new M5IOE1_Class;
        ioexp->begin();
        _io_expander[0].reset(ioexp);
      }
      break;
#elif defined (CONFIG_IDF_TARGET_ESP32C5)
    case board_t::board_M5ToughC5:
      { /// LCD 電源/リセット/バックライトのほか TF 電源 (PYG6) と
        /// TF カード検出 (PYG14) がこの IOE にぶら下がる
        auto ioexp = new M5IOE1_Class;
        ioexp->begin();
        _io_expander[0].reset(ioexp);
      }
      break;
#elif defined (CONFIG_IDF_TARGET_ESP32C61)
    case board_t::board_M5CoreMatrix:
      { /// Controls the LED matrix / TF / Grove / buzzer power rails,
        /// the charge current selector and the buzzer PWM.
        auto ioexp = new M5IOE1_Class;
        ioexp->begin();
        _io_expander[0].reset(ioexp);
      }
      break;
#endif
    default:
      break;
    }
#endif
  }
  void M5Unified::_setup_led(board_t board)
  {
#if !defined (M5UNIFIED_PC_BUILD)
    int led_count = 1;
    int byte_per_led = 3;
    int pin_power = -1;
    switch (board)
    {
#if defined (CONFIG_IDF_TARGET_ESP32S3)
    case board_t::board_M5PowerHub:
      Led.setLedInstance(std::make_shared<m5::LED_PowerHub_Class>());
      return;
    case board_t::board_M5StampS3Bat:
    {
      auto busled = std::make_shared<m5::LED_PMIC_Class>();
      auto buscfg = busled->getConfig();
      buscfg.pin_data = 0;
      buscfg.led_count = 1;
      busled->setConfig(buscfg);
      Led.setLedInstance(busled);
      return;
    }
    case board_t::board_M5PaperMono:
      Led.setLedInstance(std::make_shared<m5::LED_PaperMono_Class>());
      return;

    case board_t::board_M5PaperColor:
      led_count = 2;
      break;
#elif defined (CONFIG_IDF_TARGET_ESP32C6)
    case board_t::board_M5NanoC6:
      pin_power = GPIO_NUM_19;  // RGB LED power switch
      break;
#elif defined (CONFIG_IDF_TARGET_ESP32H2)
    case board_t::board_M5NanoH2:
      pin_power = GPIO_NUM_10;  // RGB LED power switch
      break;
#else
    case board_t::board_M5AtomMatrix:
      led_count = 25;
      break;
#endif
    default:
      break;
    }

    auto pin_rgb_led = M5.getPin(m5::pin_name_t::rgb_led); //Line: 181
    if (pin_rgb_led >= 0)
    {
      auto busled = std::make_shared<m5::LedBus_RMT>();
      auto buscfg = busled->getConfig();
      buscfg.pin_data = pin_rgb_led;
      buscfg.pin_power = pin_power;
      busled->setConfig(buscfg);
      auto led_strip = std::make_shared<m5::LED_Strip_Class>();
      auto ledcfg = led_strip->getConfig();
      ledcfg.led_count = led_count;
      ledcfg.byte_per_led = byte_per_led;
      led_strip->setBus(busled);
      led_strip->setConfig(ledcfg);
      Led.setLedInstance(led_strip);
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
     || pmic_type == Power_Class::pmic_t::pmic_axp192
     || pmic_type == Power_Class::pmic_t::pmic_m5pm1)
    {
      _use_pmic_button = cfg.pmic_button;
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
    case board_t::board_M5AtomVoice:
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
    case board_t::board_M5AtomVoice:
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

#elif defined (CONFIG_IDF_TARGET_ESP32H2)

    case board_t::board_M5NanoH2:
      m5gfx::pinMode(GPIO_NUM_9, m5gfx::pin_mode_t::input_pullup);
      break;

#elif defined (CONFIG_IDF_TARGET_ESP32S3)
    case board_t::board_M5AtomS3:
    case board_t::board_M5AtomS3Lite:
    case board_t::board_M5AtomS3U:
    case board_t::board_M5AtomS3R:
    case board_t::board_M5AtomVoiceS3R:
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
    case board_t::board_M5CardputerADV:
      m5gfx::pinMode(GPIO_NUM_0, m5gfx::pin_mode_t::input);
      break;

    case board_t::board_M5Capsule:
      {
        // Board detection leaves stray edges on the Port A lines. Issue an I2C STOP so
        // a device on the port (e.g. UnitHEART MAX30100) does not stay mid-transfer,
        // then release the pins: left as push-pull outputs they keep driving High, and
        // drivers that only enable the input (e.g. RMT RX on ESP-IDF 5.4+) cannot see the line.
        m5gfx::gpio::pin_backup_t grove_backup[] = { GPIO_NUM_15, GPIO_NUM_13 };
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
        for (auto &backup : grove_backup) {
          backup.restore();
        }
      }
      m5gfx::pinMode(GPIO_NUM_42, m5gfx::pin_mode_t::input);
      break;

    case board_t::board_M5Dial:
    case board_t::board_M5DinMeter:
      m5gfx::pinMode(GPIO_NUM_42, m5gfx::pin_mode_t::input);
      break;

    case board_t::board_M5StampPLC:
      {
        auto& ioexp = getIOExpander(0);
        // lcd backlight
        ioexp.setDirection(7, true);
        ioexp.setPullMode(7, IOExpander_Base::pull_down);
        ioexp.setHighImpedance(7, false);
  
        for (int i = 0; i < 3; ++i) {
          // button a~c
          ioexp.setDirection(i, false);
          ioexp.setPullMode(i, IOExpander_Base::pull_up);
          ioexp.setHighImpedance(i, false);
        }
        delay(100);
      }
      break;

    case board_t::board_M5PowerHub:
      m5gfx::pinMode(GPIO_NUM_11, m5gfx::pin_mode_t::input);
      break;

    case board_t::board_M5DualKey:
      m5gfx::pinMode(GPIO_NUM_0, m5gfx::pin_mode_t::input);
      m5gfx::pinMode(GPIO_NUM_17, m5gfx::pin_mode_t::input);
      break;

    case board_t::board_M5StickS3:
      m5gfx::pinMode(GPIO_NUM_11, m5gfx::pin_mode_t::input);
      m5gfx::pinMode(GPIO_NUM_12, m5gfx::pin_mode_t::input);
      // PA Control Pin Init
      this->In_I2C.bitOff(m5pm1_i2c_addr, 0x16, 1 << 3, 100000); // Set pin gpio3 as gpio function
      this->In_I2C.bitOn(m5pm1_i2c_addr, 0x10, 1 << 3, 100000);  // Set pin gpio3 mode: output
      this->In_I2C.bitOff(m5pm1_i2c_addr, 0x13, 1 << 3, 100000); // Set gpio3 push-pull mode
      this->In_I2C.bitOff(m5pm1_i2c_addr, 0x11, 1 << 3, 100000); // Set gpio3 output low
      break;

    case board_t::board_M5PaperDIY:
      m5gfx::pinMode(GPIO_NUM_4, m5gfx::pin_mode_t::input);
      m5gfx::pinMode(GPIO_NUM_3, m5gfx::pin_mode_t::input);
      break;

    case board_t::board_M5PaperColor:
      m5gfx::pinMode(GPIO_NUM_1, m5gfx::pin_mode_t::input);
      m5gfx::pinMode(GPIO_NUM_9, m5gfx::pin_mode_t::input);
      m5gfx::pinMode(GPIO_NUM_10, m5gfx::pin_mode_t::input);
      break;

    case board_t::board_M5ChainCaptain:
      m5gfx::pinMode(GPIO_NUM_1, m5gfx::pin_mode_t::input);
      m5gfx::pinMode(GPIO_NUM_4, m5gfx::pin_mode_t::input);
      m5gfx::pinMode(GPIO_NUM_5, m5gfx::pin_mode_t::input);
      // Audio PA -- G21
      m5gfx::pinMode(GPIO_NUM_21, m5gfx::pin_mode_t::output);
      m5gfx::gpio_lo(GPIO_NUM_21);
      // Audio Power -- M5IO1_G5
      {
        auto& ioe1 = getIOExpander(0);
        ioe1.setHighImpedance(M5IOE1_Class::gpio5, false);
        ioe1.setDirection(M5IOE1_Class::gpio5, true);
        ioe1.digitalWrite(M5IOE1_Class::gpio5, false);
      }
      break;

    case board_t::board_M5PaperMono:
      m5gfx::pinMode(GPIO_NUM_2, m5gfx::pin_mode_t::input);
      m5gfx::pinMode(GPIO_NUM_3, m5gfx::pin_mode_t::input);
      break;

    case board_t::board_M5StopWatch:
      m5gfx::pinMode(GPIO_NUM_1, m5gfx::pin_mode_t::input);
      m5gfx::pinMode(GPIO_NUM_2, m5gfx::pin_mode_t::input);
      // M5IOE1 PIN4: display power
      {
        auto& ioe1 = getIOExpander(0);
        ioe1.setHighImpedance(M5IOE1_Class::gpio4, false);
        ioe1.setDirection(M5IOE1_Class::gpio4, true);
        ioe1.digitalWrite(M5IOE1_Class::gpio4, true);
      }
      // M5IOE1_G3 codec power / M5IOE1_G10 PA
      {
        auto& ioe1 = getIOExpander(0);
        ioe1.setHighImpedance(M5IOE1_Class::gpio3, false);
        ioe1.setHighImpedance(M5IOE1_Class::gpio10, false);
        ioe1.setDirection(M5IOE1_Class::gpio3, true);
        ioe1.setDirection(M5IOE1_Class::gpio10, true);
        ioe1.digitalWrite(M5IOE1_Class::gpio3, false);
        ioe1.digitalWrite(M5IOE1_Class::gpio10, false);
      }
      break;

#elif defined (CONFIG_IDF_TARGET_ESP32P4)
    case board_t::board_M5CoreP4X:
      {
        auto& ioe1 = getIOExpander(0);
        ioe1.setHighImpedance(M5IOE1_Class::gpio1, false);
        ioe1.setHighImpedance(M5IOE1_Class::gpio3, false);
        ioe1.setDirection(M5IOE1_Class::gpio1, true);
        ioe1.setDirection(M5IOE1_Class::gpio3, true);
        ioe1.digitalWrite(M5IOE1_Class::gpio1, false);
        ioe1.digitalWrite(M5IOE1_Class::gpio3, false);
      }
      break;

    case board_t::board_M5UnitPoEP4:
      m5gfx::pinMode(GPIO_NUM_45, m5gfx::pin_mode_t::input);
      break;

    case board_t::board_M5Tab5:
    case board_t::board_M5Tab5X:
#if defined (ARDUINO)
      // 汎用 esp32p4 ボード選択時、core 既定の SDIO ピンでは C6 (WiFi/BLE) に届かない。
      // WiFi.begin より前 (ESP-Hosted 初期化前) に Tab5 の配線へ差し替える。
      if (hostedSetPins && !(hostedIsInitialized && hostedIsInitialized()))
      {
        hostedSetPins(GPIO_NUM_12, GPIO_NUM_13, GPIO_NUM_11, GPIO_NUM_10, GPIO_NUM_9, GPIO_NUM_8, GPIO_NUM_15);
      }
#endif
      break;

#endif

    default:
      break;
    }

#if defined ( ARDUINO )
 #ifdef HardwareSerial_h

    if (cfg.serial_baudrate)
    { // Wait with delay to prevent startup log output from disappearing.
      delay(16);
      Serial.begin(cfg.serial_baudrate);
    }

 #endif
#endif
  }

  void M5Unified::_begin_audio(config_t& cfg)
  {
    bool(*mic_enable_cb)(void*, bool) = nullptr;
    bool(*mic_post_start_cb)(void*) = nullptr;
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
      case board_t::board_M5CoreP4X:
        if (cfg.internal_mic)
        {
          mic_cfg.pin_mck = GPIO_NUM_2;
          mic_cfg.pin_bck = GPIO_NUM_6;
          mic_cfg.pin_ws = GPIO_NUM_4;
          mic_cfg.pin_data_in = GPIO_NUM_5;
          mic_cfg.magnification = 2;
          mic_cfg.sample_rate = 24000;
          mic_cfg.input_channel = input_channel_t::input_stereo;
          mic_cfg.i2s_port = I2S_NUM_0;
          mic_enable_cb = _microphone_enabled_cb_corep4x;
        }
        break;

      case board_t::board_M5Tab5:
      case board_t::board_M5Tab5X:
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
      case board_t::board_M5StackChan:
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

      case board_t::board_M5StickS3:
        if (cfg.internal_mic)
        {
          mic_cfg.pin_mck = GPIO_NUM_18;
          mic_cfg.pin_bck = GPIO_NUM_17;
          mic_cfg.pin_ws = GPIO_NUM_15;
          mic_cfg.pin_data_in = GPIO_NUM_16;
          mic_cfg.i2s_port = I2S_NUM_1;
          mic_enable_cb = _microphone_enabled_cb_sticks3;
        }
        break;

      case board_t::board_M5PaperColor:
        if (cfg.internal_mic)
        {
          mic_cfg.over_sampling = 1;
          mic_cfg.pin_mck = GPIO_NUM_42;
          mic_cfg.pin_bck = GPIO_NUM_40;
          mic_cfg.pin_ws = GPIO_NUM_41;
          mic_cfg.pin_data_in = GPIO_NUM_39; // data in from mic output
          mic_cfg.i2s_port = I2S_NUM_1;
          mic_cfg.input_channel = input_channel_t::input_only_left;
          mic_enable_cb = _microphone_enabled_cb_papercolor;
        }
      break;

      case board_t::board_M5PaperMono:
        if (cfg.internal_mic)
        { /// builtin PDM mic
          mic_cfg.pin_ws = GPIO_NUM_45;
          mic_cfg.pin_data_in = GPIO_NUM_46;
          mic_enable_cb = _microphone_enabled_cb_papermono;
        }
      break;

      case board_t::board_M5StopWatch:
        if (cfg.internal_mic)
        {
          mic_cfg.pin_mck = GPIO_NUM_18;
          mic_cfg.pin_bck = GPIO_NUM_17;
          mic_cfg.pin_ws = GPIO_NUM_15;
          mic_cfg.pin_data_in = GPIO_NUM_16;
          mic_cfg.i2s_port = I2S_NUM_1;
          mic_enable_cb = _microphone_enabled_cb_stopwatch;
          mic_post_start_cb = _microphone_post_start_cb_stopwatch;
        }
      break;

      case board_t::board_M5ChainCaptain:
        if (cfg.internal_mic)
        {
          mic_cfg.pin_mck = GPIO_NUM_40;
          mic_cfg.pin_bck = GPIO_NUM_38;
          mic_cfg.pin_ws = GPIO_NUM_41;
          mic_cfg.pin_data_in = GPIO_NUM_39;
          mic_cfg.i2s_port = I2S_NUM_1;
          mic_cfg.sample_rate = 16000;
          mic_enable_cb = _microphone_enabled_cb_chain_captain;
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

      case board_t::board_M5CardputerADV:
        if (cfg.internal_mic)
        {
          mic_cfg.pin_data_in = GPIO_NUM_46;
          mic_cfg.pin_ws = GPIO_NUM_43;
          mic_cfg.pin_bck = GPIO_NUM_41;
          mic_enable_cb = _microphone_enabled_cb_cardputer_adv;
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

      case board_t::board_M5AtomVoice:
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
#if defined M5UNIFIED_I2S_PORT_COUNT
      spk_cfg.i2s_port = (i2s_port_t)(M5UNIFIED_I2S_PORT_COUNT - 1);
#else
      spk_cfg.i2s_port = (i2s_port_t)(I2S_NUM_MAX - 1);
#endif
      switch (_board)
      {
#if defined (M5UNIFIED_PC_BUILD)
#elif defined (CONFIG_IDF_TARGET_ESP32C6)
      case board_t::board_M5UnitC6L:
      case board_t::board_ArduinoNessoN1:
        if (cfg.internal_spk)
        {
          spk_cfg.pin_data_out = GPIO_NUM_11;
          spk_cfg.buzzer = true;
          spk_cfg.magnification = 48;
        }
        break;

#elif defined (CONFIG_IDF_TARGET_ESP32P4)
      case board_t::board_M5CoreP4X:
        if (cfg.internal_spk)
        {
          spk_cfg.pin_mck = GPIO_NUM_2;
          spk_cfg.pin_bck = GPIO_NUM_6;
          spk_cfg.pin_ws = GPIO_NUM_4;
          spk_cfg.pin_data_out = GPIO_NUM_3;
          spk_cfg.magnification = 4;
          spk_cfg.sample_rate = 24000;
          spk_cfg.i2s_port = I2S_NUM_0;
          spk_enable_cb = _speaker_enabled_cb_corep4x;
        }
        break;

      case board_t::board_M5Tab5:
      case board_t::board_M5Tab5X:
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
      case board_t::board_M5StackChan:
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

      case board_t::board_M5StickS3:
        if (cfg.internal_spk)
        {
          spk_cfg.pin_mck = GPIO_NUM_18;
          spk_cfg.pin_bck = GPIO_NUM_17;
          spk_cfg.pin_ws = GPIO_NUM_15;
          spk_cfg.pin_data_out = GPIO_NUM_14;
          spk_cfg.i2s_port = I2S_NUM_0;
          spk_cfg.magnification = 1;
          spk_cfg.sample_rate = 22050;
          spk_cfg.stereo = true;
          spk_cfg.buzzer = false;
          spk_cfg.use_dac = false;
          spk_cfg.dac_zero_level = 0;
          spk_enable_cb = _speaker_enabled_cb_sticks3;
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

      case board_t::board_M5AtomVoiceS3R:
        if (cfg.internal_mic) {
          cfg.internal_imu = false;

          // spk_cfg.pin_mck = GPIO_NUM_11;
          spk_cfg.pin_bck = GPIO_NUM_17;
          spk_cfg.pin_ws = GPIO_NUM_3;
          spk_cfg.pin_data_out = GPIO_NUM_48;
          spk_cfg.magnification = 1;
          spk_cfg.i2s_port = I2S_NUM_1;
          spk_enable_cb = _speaker_enabled_cb_atom_echos3r;

          mic_cfg.i2s_port = spk_cfg.i2s_port;
          mic_cfg.pin_mck = GPIO_NUM_11;
          mic_cfg.pin_bck = GPIO_NUM_17;
          mic_cfg.pin_ws = GPIO_NUM_3;
          mic_cfg.pin_data_in = GPIO_NUM_4;
          mic_cfg.magnification = 1;
          mic_cfg.over_sampling = 1;
          mic_cfg.stereo = true;
          mic_enable_cb = _microphone_enabled_cb_atom_echos3r;
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

      case board_t::board_M5PaperMono:
        if (cfg.internal_spk)
        {
          spk_cfg.pin_data_out = GPIO_NUM_42;
          spk_cfg.buzzer = true;
          spk_cfg.magnification = 48;
        }
        break;

      case board_t::board_M5PaperColor:
        if (cfg.internal_spk)
        {
          spk_cfg.pin_mck = GPIO_NUM_42;
          spk_cfg.pin_bck = GPIO_NUM_40;
          spk_cfg.pin_ws = GPIO_NUM_41;
          spk_cfg.pin_data_out = GPIO_NUM_38; // data out to spk
          spk_cfg.i2s_port = I2S_NUM_0;
          spk_cfg.magnification = 1;
          spk_cfg.sample_rate = 44100;
          spk_cfg.stereo = true;
          spk_enable_cb = _speaker_enabled_cb_papercolor;
        }
        break;

      case board_t::board_M5StopWatch:
        if (cfg.internal_spk)
        {
          spk_cfg.pin_mck = GPIO_NUM_18;
          spk_cfg.pin_bck = GPIO_NUM_17;
          spk_cfg.pin_ws = GPIO_NUM_15;
          spk_cfg.pin_data_out = GPIO_NUM_21;
          spk_cfg.i2s_port = I2S_NUM_0;
          spk_cfg.magnification = 4;
          spk_cfg.sample_rate = 44100;
          spk_cfg.stereo = true;
          spk_cfg.buzzer = false;
          spk_cfg.use_dac = false;
          spk_cfg.dac_zero_level = 0;
          spk_enable_cb = _speaker_enabled_cb_stopwatch;
        }
      break;

      case board_t::board_M5ChainCaptain:
        if (cfg.internal_spk)
        {
          spk_cfg.pin_mck = GPIO_NUM_40;
          spk_cfg.pin_bck = GPIO_NUM_38;
          spk_cfg.pin_ws = GPIO_NUM_41;
          spk_cfg.pin_data_out = GPIO_NUM_42;
          spk_cfg.i2s_port = I2S_NUM_0;
          spk_cfg.magnification = 1;
          spk_cfg.sample_rate = 44100;
          spk_cfg.stereo = true;
          spk_cfg.buzzer = false;
          spk_cfg.use_dac = false;
          spk_cfg.dac_zero_level = 0;
          spk_enable_cb = _speaker_enabled_cb_chain_captain;
        }
      break;

      case board_t::board_M5Cardputer:
      case board_t::board_M5CardputerADV:
        if (cfg.internal_spk)
        {
          spk_cfg.pin_bck = GPIO_NUM_41;
          spk_cfg.pin_ws = GPIO_NUM_43;
          spk_cfg.pin_data_out = GPIO_NUM_42;
          spk_cfg.magnification = 16;
          spk_cfg.i2s_port = I2S_NUM_1;
          if (_board == board_t::board_M5CardputerADV) {
            spk_enable_cb = _speaker_enabled_cb_cardputer_adv;
          }
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

      case board_t::board_M5AtomVoice:
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
        if (_board == board_t::board_M5Tab5
         || _board == board_t::board_M5Tab5X
         || _board == board_t::board_M5CoreP4X)
#elif defined ( CONFIG_IDF_TARGET_ESP32S3 )
 #define ENABLE_M5MODULE
        if (_board == board_t::board_M5StackCoreS3
         || _board == board_t::board_M5StackCoreS3SE
         || _board == board_t::board_M5StackChan)
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
      Mic.setPostStartCallback(this, mic_post_start_cb);
      Mic.config(mic_cfg);
    }
    if (spk_cfg.pin_data_out >= 0)
    {
      Speaker.setCallback(this, spk_enable_cb);
      Speaker.config(spk_cfg);
    }
  }

  bool M5Unified::_clearWakeupInterrupt(void)
  {
    bool res = true;
    // A touch panel holds its INT asserted until the touch data is read. Every board that
    // uses the touch INT as its wakeup pin therefore has to consume the data here.
    // ( M5Paper = GPIO36 , M5PaperS3 = GPIO48 , Core2 / Tough = GPIO39 , CoreS3 = via AW9523 )
    if (!_displays.empty() && _displays.front().touch())
    { // Same source as Touch_Class, see Touch.begin() in _begin().
      m5gfx::touch_point_t tp;
      _displays.front().getTouchRaw(&tp, 1);
    }

#if defined (CONFIG_IDF_TARGET_ESP32S3)
    switch (getBoard())
    {
    case board_t::board_M5StackCoreS3:
    case board_t::board_M5StackCoreS3SE:
    case board_t::board_M5StackChan:
      { // TOUCH_INT -> AW9523 P1_2 -> AW9523 INTN -> I2C_INT -> GPIO21.
        // The AW9523 reports input changes only, so releasing the touch INT is not enough:
        // its input would stay in the touched state and a later touch would not produce
        // any change, leaving the wakeup source dead.
        uint8_t buf[2];
        res = In_I2C.readRegister(aw9523_i2c_addr, 0x00, buf, sizeof(buf), 400000);
      }
      break;

    default:
      break;
    }
#elif defined (CONFIG_IDF_TARGET_ESP32C5)
    switch (getBoard())
    {
    case board_t::board_M5ToughC5:
      { // TOUCH_INT や RTC_INT は PM1 に集約され、PM1 の IRQ 出力 -> GPIO4 が
        // 唯一の wakeup ピンになる。IRQ 出力は IRQ ステータス (0x40-0x42) が
        // 全て 0 になるまで Low を保つため、ここでクリアして解放する。
        // WAKE_SRC が残っていると IRQ Status 3 の WAKEUP ビットが再セット
        // され続けるので、先に WAKE_SRC を消す。
        res  = Power.M5pm1.clearWakeSource();
        res &= Power.M5pm1.clearIRQStatus();
      }
      break;

    default:
      break;
    }
#elif defined (CONFIG_IDF_TARGET_ESP32C61)
    switch (getBoard())
    {
    case board_t::board_M5CoreMatrix:
      { // KEY and IMU wake events are funneled into the PM1, whose IRQ output
        // (GPIO2) is the only wakeup pin. The IRQ output stays low until every
        // IRQ status bit is cleared, so clear them here to release the pin.
        // Clear WAKE_SRC first: while it is set, the WAKEUP bit of IRQ status 3
        // keeps getting re-asserted.
        res  = Power.M5pm1.clearWakeSource();
        res &= Power.M5pm1.clearIRQStatus();
      }
      break;

    default:
      break;
    }
#endif
    return res;
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
      M5.Rtc.begin(&M5.In_I2C, M5.getBoard());
    }
    if (!M5.Rtc.isEnabled() && cfg.external_rtc && Ex_I2C.isEnabled())
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
    if (!M5.Imu.isEnabled() && cfg.external_imu && Ex_I2C.isEnabled())
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
      case board_t::board_M5ToughC5:
      case board_t::board_M5StackCoreS3SE:
      case board_t::board_M5StackCoreS3:
      case board_t::board_M5StackChan:
        tb_y = 240;
        tb_k = 614; // (65536*3/320)
        break;
      case board_t::board_M5Paper:
      case board_t::board_M5PaperS3:
        tb_y = 960;
        tb_k = 364; // (65536*3/540)
        break;
      case board_t::board_M5Tab5:
      case board_t::board_M5Tab5X:
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
    case board_t::board_M5AtomVoice:
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
    case board_t::board_M5CardputerADV:
      use_rawstate_bits = 0b00001;
      btn_rawstate_bits = (!m5gfx::gpio_in(GPIO_NUM_0)) & 1;
      break;

    case board_t::board_M5AtomS3:
    case board_t::board_M5AtomS3Lite:
    case board_t::board_M5AtomS3U:
    case board_t::board_M5AtomS3R:
    case board_t::board_M5AtomVoiceS3R:
      use_rawstate_bits = 0b00001;
      btn_rawstate_bits = (!m5gfx::gpio_in(GPIO_NUM_41)) & 1;
      break;

    case board_t::board_M5Capsule:
    case board_t::board_M5Dial:
    case board_t::board_M5DinMeter:
      use_rawstate_bits = 0b00011;
      btn_rawstate_bits = ((!m5gfx::gpio_in(GPIO_NUM_42)) & 1)
                        | ((!m5gfx::gpio_in(GPIO_NUM_0)) & 1) << 1;
      break;

    case board_t::board_M5StampPLC:
    {
      use_rawstate_bits = 0b00111;
      uint8_t value = 0xFF;
      if (_io_expander[0]->readRegister(0x0F, &value, 1)) {
        btn_rawstate_bits = (!(value & 0b100) ? 0b00001 : 0) // BtnA
                          | (!(value & 0b010) ? 0b00010 : 0) // BtnB
                          | (!(value & 0b001) ? 0b00100 : 0) // BtnC
                          ;
      }
    }
      break;

    case board_t::board_M5PowerHub:
    {
      use_rawstate_bits = 0b00011;
      auto value = M5.In_I2C.readRegister8(powerhub_i2c_addr, 0xA0, 100000);
      // ESP_LOGI("M5Unified", "M5PowerHub Btn read: %02X", value);
      btn_rawstate_bits = (!m5gfx::gpio_in(GPIO_NUM_11) & 1) // BtnA
                        | (!(value & 0b001) ? 0b00010 : 0) // BtnB
                        ;
      break;
    }

    case board_t::board_M5DualKey:
      use_rawstate_bits = 0b00011;
      btn_rawstate_bits = ((!m5gfx::gpio_in(GPIO_NUM_17)) & 1)
                        | ((!m5gfx::gpio_in(GPIO_NUM_0)) & 1) << 1;
      break;

    case board_t::board_M5StickS3:
      use_rawstate_bits = 0b00011;
      btn_rawstate_bits = ((!m5gfx::gpio_in(GPIO_NUM_11)) & 1)
                        | ((!m5gfx::gpio_in(GPIO_NUM_12)) & 1) << 1;
      break;

    case board_t::board_M5PaperDIY:
      use_rawstate_bits = 0b00011;
      btn_rawstate_bits = ((!m5gfx::gpio_in(GPIO_NUM_4)) & 1)
                        | ((!m5gfx::gpio_in(GPIO_NUM_3)) & 1) << 1;
      break;

    case board_t::board_M5PaperColor:
      use_rawstate_bits = 0b00111;
      btn_rawstate_bits = ((!m5gfx::gpio_in(GPIO_NUM_10)) & 1)
                        | ((!m5gfx::gpio_in(GPIO_NUM_9)) & 1) << 1
                        | ((!m5gfx::gpio_in(GPIO_NUM_1)) & 1) << 2;
      break;

    case board_t::board_M5ChainCaptain:
      use_rawstate_bits = 0b00111;
      btn_rawstate_bits = ((!m5gfx::gpio_in(GPIO_NUM_1)) & 1)
                        | ((!m5gfx::gpio_in(GPIO_NUM_4)) & 1) << 1
                        | ((!m5gfx::gpio_in(GPIO_NUM_5)) & 1) << 2;
      break;

    case board_t::board_M5PaperMono:
      use_rawstate_bits = 0b00011;
      btn_rawstate_bits = ((!m5gfx::gpio_in(GPIO_NUM_2)) & 1)
                        | ((!m5gfx::gpio_in(GPIO_NUM_3)) & 1) << 1;
      break;

    case board_t::board_M5StopWatch:
      use_rawstate_bits = 0b00011;
      btn_rawstate_bits = ((!m5gfx::gpio_in(GPIO_NUM_2)) & 1)
                        | ((!m5gfx::gpio_in(GPIO_NUM_1)) & 1) << 1;
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

    case board_t::board_M5UnitC6L:
      {
        use_rawstate_bits = 0b00001;
        uint8_t value = 0xFF;
        if (_io_expander[0]->readRegister(0x0F, &value, 1)) {
          btn_rawstate_bits = (!(value & 0b001) ? 0b00001 : 0); // BtnA
        }
      }
      break;

    case board_t::board_ArduinoNessoN1:
      {
        use_rawstate_bits = 0b00011;
        uint8_t value = 0xFF;
        if (_io_expander[0]->readRegister(0x0F, &value, 1)) {
            btn_rawstate_bits = (!(value & 0b001) ? 0b00001 : 0) // BtnA
                              | (!(value & 0b010) ? 0b00010 : 0) // BtnB
                              ;
        }
      }
      break;

    default:
      break;
    }

#elif defined (CONFIG_IDF_TARGET_ESP32H2)

    switch (_board)
    {
    case board_t::board_M5NanoH2:
      use_rawstate_bits = 0b00001;
      btn_rawstate_bits = (!m5gfx::gpio_in(GPIO_NUM_9) ? 0b00001 : 0);
      break;

    default:
      break;
    }

#elif defined (CONFIG_IDF_TARGET_ESP32P4)

    switch (_board)
    {
    case board_t::board_M5UnitPoEP4:
      use_rawstate_bits = 0b00001;
      btn_rawstate_bits = (!m5gfx::gpio_in(GPIO_NUM_45)) & 1;
      break;

    default:
      break;
    }

#elif defined (CONFIG_IDF_TARGET_ESP32C61)

    switch (_board)
    {
    case board_t::board_M5CoreMatrix:
      /// KEY1/2/3 are wired to PM1 GPIO0/1/2 (pressed = LOW), not to the ESP,
      /// so they are read by I2C polling. Skip the update on an I2C failure so
      /// a bus error is not reported as a button press.
      {
        uint8_t in;
        if (Power.M5pm1.getGPIOInputBits(&in))
        {
          use_rawstate_bits = 0b00111;
          btn_rawstate_bits = (~in) & 0b00111;
        }
      }
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

#if defined (CONFIG_IDF_TARGET_ESP32) || defined (CONFIG_IDF_TARGET_ESP32S3) || defined (CONFIG_IDF_TARGET_ESP32C5) || defined (CONFIG_IDF_TARGET_ESP32C61)
    if (_use_pmic_button)
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
    case board_t::board_M5ToughC5:
    case board_t::board_M5StackCoreS3SE:
    case board_t::board_M5StackCoreS3:
    case board_t::board_M5StackChan:
      height = 240;
      break;
    case board_t::board_M5Paper:
    case board_t::board_M5PaperS3:
      height = 960;
      break;
    case board_t::board_M5Tab5:
    case board_t::board_M5Tab5X:
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
