cmake_minimum_required(VERSION 3.25)
project(img_to_ascii)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED
        libavformat
        libavcodec
        libswscale
        libavutil
)

add_executable(img_to_ascii
        src/main.cpp
        src/Renderer.cpp
        include/Renderer.h
        src/GenerateFrames.cpp
        include/dataStructures.h
        src/miniaudio.c
        include/AudioPlayer.h
        include/SyncClock.h
)

target_include_directories(img_to_ascii PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${FFMPEG_INCLUDE_DIRS}
)

target_link_libraries(img_to_ascii PRIVATE
        ${FFMPEG_LIBRARIES}
)

target_compile_options(img_to_ascii PRIVATE
        ${FFMPEG_CFLAGS_OTHER}
)

target_compile_options(img_to_ascii PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Werror
)