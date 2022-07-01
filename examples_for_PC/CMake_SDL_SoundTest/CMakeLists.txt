cmake_minimum_required (VERSION 3.8)
project(M5Unified_SDL)

add_definitions(-DLGFX_SDL)

file(GLOB Target_Files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS 
    *.cpp
    ../../../M5GFX/src/*.cpp
    ../../../M5GFX/src/lgfx/Fonts/efont/*.c
    ../../../M5GFX/src/lgfx/Fonts/IPA/*.c
    ../../../M5GFX/src/lgfx/utility/*.c
    ../../../M5GFX/src/lgfx/v1/*.cpp
    ../../../M5GFX/src/lgfx/v1/misc/*.cpp
    ../../../M5GFX/src/lgfx/v1/panel/Panel_Device.cpp
    ../../../M5GFX/src/lgfx/v1/platforms/sdl/*.cpp
    ../../../M5Unified/src/*.cpp
    ../../../M5Unified/src/utility/*.cpp
    ../../../LGFX_SDL_extension/src/*.c
    ../../../LGFX_SDL_extension/src/*.cpp
    )
add_executable (M5Unified_SDL ${Target_Files})

target_compile_features(M5Unified_SDL PUBLIC cxx_std_17)
target_include_directories(M5Unified_SDL PUBLIC "../../../M5Unified/src/")
target_include_directories(M5Unified_SDL PUBLIC "../../../M5GFX/src/")
target_include_directories(M5Unified_SDL PUBLIC "../../../LGFX_SDL_extension/src/")

IF (CMAKE_SYSTEM_NAME MATCHES "Linux")

    find_package(SDL2 REQUIRED SDL2)
    include_directories(${SDL2_INCLUDE_DIRS})
    target_link_libraries(M5Unified_SDL -lpthread ${SDL2_LIBRARIES})

ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Windows")

    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus>")

    target_include_directories(M5Unified_SDL PUBLIC "C:/SDL2/include")
    target_link_libraries(M5Unified_SDL PUBLIC "C:/SDL2/out/build/x64-Debug/SDL2d.lib")
    target_link_libraries(M5Unified_SDL PUBLIC "C:/SDL2/out/build/x64-Debug/SDL2maind.lib")

ENDIF ()
