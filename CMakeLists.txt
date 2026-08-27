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



message(STATUS "M5Unified use component = ${COMPONENT_REQUIRES}" )


register_component()

# Arduino as an ESP-IDF component: arduino-esp32 publishes -DARDUINO... as PUBLIC compile options, so
# they only reach components that link against it, while the public headers of this library change
# class layout with ARDUINO. Link the Arduino component publicly whenever it is part of the build so
# this library is compiled in the same mode as the application. Only components already selected
# for the build are considered; a build without Arduino is unaffected.
#   M5UNIFIED_ARDUINO_COMPONENT=<name>  use a differently named Arduino component
#   M5UNIFIED_ARDUINO_COMPONENT=OFF     disable the automatic dependency
set(_m5unified_arduino_candidates arduino arduino-esp32 espressif__arduino-esp32)
set(_m5unified_arduino_enabled ON)
if(DEFINED M5UNIFIED_ARDUINO_COMPONENT AND NOT "${M5UNIFIED_ARDUINO_COMPONENT}" STREQUAL "")
    if(M5UNIFIED_ARDUINO_COMPONENT)
        set(_m5unified_arduino_candidates ${M5UNIFIED_ARDUINO_COMPONENT})
    else()
        set(_m5unified_arduino_enabled OFF)
    endif()
endif()
if(_m5unified_arduino_enabled)
    idf_build_get_property(_m5unified_arduino_build_components BUILD_COMPONENTS)
    set(_m5unified_arduino_hits)
    foreach(_m5unified_arduino_name ${_m5unified_arduino_candidates})
        if(_m5unified_arduino_name IN_LIST _m5unified_arduino_build_components)
            list(APPEND _m5unified_arduino_hits ${_m5unified_arduino_name})
        endif()
    endforeach()
    list(LENGTH _m5unified_arduino_hits _m5unified_arduino_count)
    if(_m5unified_arduino_count GREATER 1)
        message(FATAL_ERROR "M5UNIFIED: several Arduino components are in the build (${_m5unified_arduino_hits}). Set M5UNIFIED_ARDUINO_COMPONENT to the one to use.")
    elseif(_m5unified_arduino_count EQUAL 1)
        idf_component_get_property(_m5unified_arduino_lib ${_m5unified_arduino_hits} COMPONENT_LIB)
        target_link_libraries(${COMPONENT_LIB} PUBLIC ${_m5unified_arduino_lib})
    endif()
endif()
