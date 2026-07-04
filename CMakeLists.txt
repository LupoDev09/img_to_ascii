cmake_minimum_required(VERSION 3.25)
project(img_to_ascii)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# ---------------------------------------------------------
# Hauptprogramm
# ---------------------------------------------------------
add_executable(img_to_ascii
        src/main.cpp
        include/Data_structures/Pixel.h
        include/Data_structures/Frame.h
        include/Data_structures/FrameSource.h
        include/Downscaler.h
        include/AsciiMapper.h
        include/Renderer.h
)

target_include_directories(img_to_ascii PRIVATE ${CMAKE_SOURCE_DIR}/include)
