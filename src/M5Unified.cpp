// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// This library is built as a single translation unit: this file includes every
// implementation file (*.inl), and those are not compiled on their own.
//
// Why: every implementation file pulls in the same framework headers, and parsing them
// once instead of once per file cuts the library build time several-fold (the Arduino
// build also pre-scans each source file for includes, which doubled that cost).
//
// Maintenance notes:
// - Adding an implementation file: create it as *.inl and add an #include below. Do not
//   add *.cpp files under src/.
// - File-local names are visible to every file that follows: static functions and
//   constants, macros, and anonymous namespaces do not isolate files here. Use a name that
//   identifies the file (or a class member), and #undef helper macros at the end of the
//   file that defines them. Constants shared by several files go in a header
//   (utility/m5unified_i2c_addr.hpp for the on-board I2C addresses).
// - M5Unified.inl (the former M5Unified.cpp) comes last; it uses the classes above.

#define __STDC_FORMAT_MACROS

#define M5UNIFIED_IMPLEMENTATION

#include "utility/Button_Class.inl"
#include "utility/I2C_Class.inl"
#include "utility/Log_Class.inl"
#include "utility/M5Timer.inl"
#include "utility/Touch_Class.inl"
#include "utility/M5IOE1_Class.inl"
#include "utility/PI4IOE5V6408_Class.inl"
#include "utility/LED_Class.inl"
#include "utility/led/LED_PMIC_Class.inl"
#include "utility/led/LED_PaperMono_Class.inl"
#include "utility/led/LED_PowerHub_Class.inl"
#include "utility/led/LED_Strip_Class.inl"
#include "utility/IMU_Class.inl"
#include "utility/imu/IMU_Base.inl"
#include "utility/imu/AK8963_Class.inl"
#include "utility/imu/BMI270_Class.inl"
#include "utility/imu/BMM150_Class.inl"
#include "utility/imu/MPU6886_Class.inl"
#include "utility/imu/SH200Q_Class.inl"
#include "utility/RTC_Class.inl"
#include "utility/rtc/RTC_Base.inl"
#include "utility/rtc/PCF8563_Class.inl"
#include "utility/rtc/RTC_PowerHub_Class.inl"
#include "utility/rtc/RX8130_Class.inl"
#include "utility/Power_Class.inl"
#include "utility/power/AW32001_Class.inl"
#include "utility/power/AXP192_Class.inl"
#include "utility/power/AXP2101_Class.inl"
#include "utility/power/BQ27220_Class.inl"
#include "utility/power/INA226_Class.inl"
#include "utility/power/INA3221_Class.inl"
#include "utility/power/IP5306_Class.inl"
#include "utility/power/M5PM1_Class.inl"
#include "utility/Speaker_Class.inl"
#include "utility/Mic_Class.inl"
#include "M5Unified.inl"

#undef M5UNIFIED_IMPLEMENTATION
