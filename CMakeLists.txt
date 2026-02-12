cmake_minimum_required(VERSION 3.25)
project(img_to_ascii)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------------------------------------------------------
# Hauptprogramm
# ---------------------------------------------------------
add_executable(img_to_ascii
        src/main.cpp
)

target_include_directories(img_to_ascii PRIVATE ${CMAKE_SOURCE_DIR}/include)

execute_process(
  COMMAND ${CMAKE_COMMAND} -E create_symlink
          ${CMAKE_BINARY_DIR}/compile_commands.json
          ${CMAKE_SOURCE_DIR}/compile_commands.json
)
