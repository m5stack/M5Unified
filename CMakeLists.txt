set(COMPONENT_ADD_INCLUDEDIRS
    src
    )
file(GLOB SRCS
     src/*.cpp
     src/utility/*.cpp
     )
set(COMPONENT_SRCS ${SRCS})
set(COMPONENT_REQUIRES M5GFX esp_adc_cal)

### If you use arduino-esp32 components, please activate next comment line.
# list(APPEND COMPONENT_REQUIRES arduino-esp32)


message(STATUS "M5Unified use component = ${COMPONENT_REQUIRES}" )


register_component()

