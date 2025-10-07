cmake_minimum_required(VERSION 3.25)
project(img_to_ascii)

set(CMAKE_CXX_STANDARD 20)

add_executable(img_to_ascii main.cpp stb_image.h)
