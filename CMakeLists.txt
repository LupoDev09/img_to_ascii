cmake_minimum_required(VERSION 3.25)
project(img_to_ascii)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# ---------------------------------------------------------
# Hauptprogramm
# ---------------------------------------------------------
add_executable(img_to_ascii
        src/main.cpp
        src/Renderer.cpp
        include/Renderer.h
        src/Loader.cpp
        include/Loader.h
        include/stb_image_implementation.h
        src/GenerateFrames.cpp
        include/GenerateFrames.h
)

target_include_directories(img_to_ascii PRIVATE ${CMAKE_SOURCE_DIR}/include)
