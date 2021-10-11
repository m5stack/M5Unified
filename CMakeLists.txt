set(COMPONENT_ADD_INCLUDEDIRS
    src
    )
file(GLOB SRCS
     src/*.cpp
     src/utility/*.cpp
     )
set(COMPONENT_SRCS ${SRCS})
set(COMPONENT_REQUIRES M5GFX esp_adc_cal)

register_component()

