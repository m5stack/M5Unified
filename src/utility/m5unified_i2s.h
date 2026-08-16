// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Internal header: I2S compatibility helpers across ESP-IDF versions.

#pragma once

#if __has_include (<sdkconfig.h>)
 #include <sdkconfig.h>
#endif
#if __has_include (<soc/soc_caps.h>)
 #include <soc/soc_caps.h>
#endif
#if __has_include (<esp_idf_version.h>)
 #include <esp_idf_version.h>
#endif

/// Number of I2S ports. (ESP-IDF v6 removed SOC_I2S_NUM; the HAL provides it as I2S_LL_INST_NUM)
#if defined (SOC_I2S_NUM)
 #define M5UNIFIED_I2S_PORT_COUNT SOC_I2S_NUM
#elif defined (ESP_IDF_VERSION) && ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 0, 0) && __has_include (<hal/i2s_ll.h>)
 #include <hal/i2s_ll.h>
 #define M5UNIFIED_I2S_PORT_COUNT I2S_LL_INST_NUM
#elif !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
 /// Cores that predate soc_caps.h (e.g. Arduino core 1.x) are always the classic ESP32.
 #define M5UNIFIED_I2S_PORT_COUNT 2
#elif defined (SOC_I2S_SUPPORTED)
 /// The target declares I2S support but the port count cannot be resolved.
 /// (Targets without I2S may include this header freely; they never reach here.)
 #error "Cannot determine the number of I2S ports for this target"
#endif

/// I2S built-in ADC/DAC capability (classic ESP32 only).
/// SOC_I2S_SUPPORTS_DAC was removed in ESP-IDF v6 (the HAL provides I2S_LL_ADC_DAC_CAPABLE
/// instead). Ancient cores may lack soc_caps.h entirely; the classic ESP32 is then
/// identified by being targetless or by CONFIG_IDF_TARGET_ESP32 (e.g. Arduino core 1.x
/// defines the target macro but has none of the capability macros).
/// (capability macros are tested for value as well: a defined-but-zero macro must not
/// enable the path)
#if (defined (SOC_I2S_SUPPORTS_DAC) && SOC_I2S_SUPPORTS_DAC) || (defined (I2S_LL_ADC_DAC_CAPABLE) && I2S_LL_ADC_DAC_CAPABLE) || !defined (CONFIG_IDF_TARGET) || defined (CONFIG_IDF_TARGET_ESP32)
 #define M5UNIFIED_I2S_ADC_DAC 1
#endif

/// I2S register layout generation. Prefer the SoC capability macro (ESP-IDF v5+).
/// On older ESP-IDF (v4.x / Arduino core 2.x) whose soc_caps.h predates
/// SOC_I2S_HW_VERSION_x, the supported chip set is fixed (ESP32/S2/S3/C3, so the
/// list below is final): enumerate the HW v1 targets (ESP32/ESP32-S2, plus
/// targetless ancient cores = classic ESP32) and treat the rest as HW v2.
#if defined (SOC_I2S_HW_VERSION_2) || defined (SOC_I2S_HW_VERSION_1)
 #if SOC_I2S_HW_VERSION_2
  #define M5UNIFIED_I2S_HW_V2 1
 #endif
#elif defined (CONFIG_IDF_TARGET) && !defined (CONFIG_IDF_TARGET_ESP32) && !defined (CONFIG_IDF_TARGET_ESP32S2)
 #define M5UNIFIED_I2S_HW_V2 1
#endif

/// The HW v2 raw register setup uses the HAL LL helpers where the header is
/// available (ESP-IDF v5+). ESP-IDF v4 has no C++-includable i2s_ll.h and writes
/// the registers directly. The include and every use site share this condition.
#if defined (M5UNIFIED_I2S_HW_V2) && __has_include (<driver/i2s_std.h>) && __has_include (<hal/i2s_ll.h>)
 #include <hal/i2s_ll.h>
 #define M5UNIFIED_I2S_USE_LL 1
#endif

/// Source clock frequency assumed by the raw clock divider setup in the speaker/mic
/// tasks (the frequency selected by tx/rx_clk_sel = 1 on HW v2, PLL_160M on HW v1).
/// This is a per-chip physical property that cannot be derived from a capability
/// macro, so every known target is enumerated explicitly.
#if defined ( CONFIG_IDF_TARGET_ESP32C3 ) || defined ( CONFIG_IDF_TARGET_ESP32C6 ) || defined ( CONFIG_IDF_TARGET_ESP32C5 ) || defined ( CONFIG_IDF_TARGET_ESP32C61 ) || defined ( CONFIG_IDF_TARGET_ESP32S3 )
 #define M5UNIFIED_I2S_PLL_D2_HZ (120*1000*1000) // 240 MHz/2
#elif defined ( CONFIG_IDF_TARGET_ESP32P4 )
 #define M5UNIFIED_I2S_PLL_D2_HZ (20*1000*1000)  // 20 MHz
#elif defined ( CONFIG_IDF_TARGET_ESP32H2 ) || defined ( CONFIG_IDF_TARGET_ESP32H4 )
 #define M5UNIFIED_I2S_PLL_D2_HZ (96*1000*1000)  // PLL_F96M
#else
 /// Unknown I2S-capable targets fall back to the HW v1 value. The message below is
 /// intentionally not #warning (which fails -Werror builds); it flags that the
 /// frequency must be verified and added to the table above.
 #if defined (M5UNIFIED_I2S_PORT_COUNT) && defined (CONFIG_IDF_TARGET) && !defined (CONFIG_IDF_TARGET_ESP32) && !defined (CONFIG_IDF_TARGET_ESP32S2)
  #pragma message ("M5Unified: unknown target, assuming a 80 MHz I2S source clock. Verify it and extend the table in m5unified_i2s.h")
 #endif
 #define M5UNIFIED_I2S_PLL_D2_HZ (80*1000*1000)  // 160 MHz/2
#endif
