cmake_minimum_required(VERSION 3.25)
project(img_to_ascii)

set(CMAKE_CXX_STANDARD 20)

add_executable(img_to_ascii
        src/main.cpp
        src/general_utils.cpp
        src/img_utils.cpp
        src/gif_utils.cpp
        src/stb_impl.cpp
)

target_include_directories(img_to_ascii PRIVATE ${CMAKE_SOURCE_DIR}/include)
