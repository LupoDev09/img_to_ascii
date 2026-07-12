cmake_minimum_required(VERSION 3.25)
project(img_to_ascii)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED
        libavformat
        libavcodec
        libavutil
        libswscale
        libswresample
)


add_executable(img_to_ascii
        src/main.cpp
        src/Renderer.cpp
        src/GenerateFrames.cpp
        src/miniaudio.c
        src/utf8_stuff.cpp
        src/AudioExtractor.cpp
        src/AudioPlayer.cpp
        include/Verbose.hpp
)

target_include_directories(img_to_ascii PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${FFMPEG_INCLUDE_DIRS}
)

target_link_libraries(img_to_ascii PRIVATE ${FFMPEG_LIBRARIES})

target_compile_options(img_to_ascii PRIVATE
        ${FFMPEG_CFLAGS_OTHER}
        -Wall
        -Wextra
        -Wpedantic
        -Werror
)
