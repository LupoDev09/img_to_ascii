cmake_minimum_required(VERSION 3.25)
project(img_to_ascii)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------
# Catch2 finden
# ---------------------------------------------------------
find_package(Catch2 3 REQUIRED)

# ---------------------------------------------------------
# Hauptprogramm
# ---------------------------------------------------------
add_executable(img_to_ascii
        src/main.cpp
        src/general_utils.cpp
        src/img_utils.cpp
        src/gif_utils.cpp
        src/stb_impl.cpp
        src/verbose.cpp
)

target_include_directories(img_to_ascii PRIVATE ${CMAKE_SOURCE_DIR}/include)

# ---------------------------------------------------------
# Tests
# ---------------------------------------------------------
# Test-Executable
add_executable(tests
        tests/test_cli.cpp
        tests/test_utils.cpp
        tests/test_invalid_inputs.cpp
        tests/test_gif_stuff.cpp
)

# Catch2 verlinken
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)

# Deine Includes auch für Tests verfügbar machen
target_include_directories(tests PRIVATE ${CMAKE_SOURCE_DIR}/include)


# ---------------------------------------------------------
# CTest-Integration aktivieren
# ---------------------------------------------------------
include(CTest)
include(Catch)
catch_discover_tests(tests)
