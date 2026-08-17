set(COMPONENT_ADD_INCLUDEDIRS
    src
    )
file(GLOB SRCS
     src/*.cpp
     src/utility/*.cpp
     src/utility/imu/*.cpp
     src/utility/led/*.cpp
     src/utility/power/*.cpp
     src/utility/rtc/*.cpp
     )
set(COMPONENT_SRCS ${SRCS})

# ESP-IDF uses the directory name as the component name (case sensitive).
# Use "M5GFX" when a sibling components/M5GFX checkout exists, otherwise "m5gfx"
# (managed_components/m5stack__m5gfx is registered as the lower-case "m5gfx").
get_filename_component(_m5u_parent "${CMAKE_CURRENT_LIST_DIR}" DIRECTORY)
if(EXISTS "${_m5u_parent}/M5GFX/CMakeLists.txt")
    set(_m5gfx_name M5GFX)
else()
    set(_m5gfx_name m5gfx)
endif()

if (IDF_VERSION_MAJOR GREATER_EQUAL 6)
    set(COMPONENT_REQUIRES ${_m5gfx_name} esp_adc driver esp_driver_gpio esp_driver_i2s esp_driver_rmt esp_driver_ledc esp_driver_touch_sens)
elseif (IDF_VERSION_MAJOR GREATER_EQUAL 5)
    set(COMPONENT_REQUIRES ${_m5gfx_name} esp_adc driver)
else()
    set(COMPONENT_REQUIRES ${_m5gfx_name} esp_adc_cal)
endif()

### If you use arduino-esp32 components, please activate next comment line.
# list(APPEND COMPONENT_REQUIRES arduino-esp32)


message(STATUS "M5Unified use component = ${COMPONENT_REQUIRES}" )


register_component()

