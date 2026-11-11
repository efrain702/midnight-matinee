cmake_minimum_required(VERSION 3.13)
project(midnight_matinee C)
set(CMAKE_C_STANDARD 99)

file(GLOB MM_SRC ${CMAKE_CURRENT_SOURCE_DIR}/engine/src/*.c)
add_executable(midnight_matinee ${MM_SRC})
target_compile_options(midnight_matinee PRIVATE -O2 -Wall -Wno-misleading-indentation)

# SDL2 + SDL2_image + SDL2_mixer: devkitPro portlibs on Wii U, system packages on PC (same setup as The Tolling)
find_package(PkgConfig REQUIRED)
if (WIIU OR COMMAND wut_create_rpx)
    pkg_check_modules(SDLX REQUIRED sdl2 SDL2_image SDL2_mixer)
    target_include_directories(midnight_matinee PRIVATE ${SDLX_STATIC_INCLUDE_DIRS})
    target_compile_options(midnight_matinee PRIVATE ${SDLX_STATIC_CFLAGS_OTHER})
    target_link_libraries(midnight_matinee PRIVATE ${SDLX_STATIC_LDFLAGS})
else()
    pkg_check_modules(SDLX REQUIRED IMPORTED_TARGET sdl2 SDL2_image SDL2_mixer)
    target_link_libraries(midnight_matinee PRIVATE PkgConfig::SDLX)
endif()
target_link_libraries(midnight_matinee PRIVATE m)

if (COMMAND wut_create_rpx)
    wut_create_rpx(midnight_matinee)
    wut_create_wuhb(midnight_matinee
        CONTENT "${CMAKE_CURRENT_SOURCE_DIR}/engine/content"
        NAME "Midnight Matinee"
        SHORTNAME "Midnight Matinee"
        AUTHOR "Junior Gomez"
        ICON "${CMAKE_CURRENT_SOURCE_DIR}/meta/icon.png"
        TVSPLASH "${CMAKE_CURRENT_SOURCE_DIR}/meta/splash_tv.png"
        DRCSPLASH "${CMAKE_CURRENT_SOURCE_DIR}/meta/splash_drc.png")
endif()
