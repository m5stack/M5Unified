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

/// I2S register layout generation. Prefer the SoC capability macro; fall back to a
/// target list for cores whose soc_caps.h predates SOC_I2S_HW_VERSION_x (e.g. ESP-IDF v4).
#if defined (SOC_I2S_HW_VERSION_2) || defined (SOC_I2S_HW_VERSION_1)
 #if SOC_I2S_HW_VERSION_2
  #define M5UNIFIED_I2S_HW_V2 1
 #endif
#elif defined ( CONFIG_IDF_TARGET_ESP32C3 ) || defined ( CONFIG_IDF_TARGET_ESP32C6 ) || defined ( CONFIG_IDF_TARGET_ESP32C5 ) || defined ( CONFIG_IDF_TARGET_ESP32C61 ) || defined ( CONFIG_IDF_TARGET_ESP32H2 ) || defined ( CONFIG_IDF_TARGET_ESP32S3 ) || defined ( CONFIG_IDF_TARGET_ESP32P4 )
 #define M5UNIFIED_I2S_HW_V2 1
#endif

/// Source clock frequency assumed by the raw clock divider setup in the speaker/mic tasks.
/// (per-chip PLL selection; this cannot be derived from a capability macro)
#if defined ( CONFIG_IDF_TARGET_ESP32C3 ) || defined ( CONFIG_IDF_TARGET_ESP32C6 ) || defined ( CONFIG_IDF_TARGET_ESP32C5 ) || defined ( CONFIG_IDF_TARGET_ESP32C61 ) || defined ( CONFIG_IDF_TARGET_ESP32S3 )
 #define M5UNIFIED_I2S_PLL_D2_HZ (120*1000*1000) // 240 MHz/2
#elif defined ( CONFIG_IDF_TARGET_ESP32P4 )
 #define M5UNIFIED_I2S_PLL_D2_HZ (20*1000*1000)  // 20 MHz
#else
 #define M5UNIFIED_I2S_PLL_D2_HZ (80*1000*1000)  // 160 MHz/2
#endif
