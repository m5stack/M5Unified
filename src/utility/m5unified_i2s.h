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
#endif
