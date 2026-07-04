cmake_minimum_required(VERSION 3.25)
project(img_to_ascii)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTS "Baue die Tests mit Catch2" ON)

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
if (BUILD_TESTS)
    find_package(Catch2 2 REQUIRED)
    add_executable(tests
            tests/test_cli.cpp
            tests/test_utils.cpp
            tests/test_invalid_inputs.cpp
            tests/test_gif_stuff.cpp
            tests/test_config.cpp
    )

    # Catch2 verlinken
    target_link_libraries(tests PRIVATE Catch2::Catch2)

    # Deine Includes auch für Tests verfügbar machen
    target_include_directories(tests PRIVATE ${CMAKE_SOURCE_DIR}/include)


    # ---------------------------------------------------------
    # CTest-Integration aktivieren
    # ---------------------------------------------------------
    include(CTest)
    include(Catch)
    catch_discover_tests(tests)
endif ()