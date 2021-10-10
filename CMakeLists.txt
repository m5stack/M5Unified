set(COMPONENT_ADD_INCLUDEDIRS
    src
    )
file(GLOB SRCS
     src/*.cpp
     src/utility/*.cpp
     )
set(COMPONENT_SRCS ${SRCS})

register_component()

