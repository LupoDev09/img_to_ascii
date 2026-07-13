cmake_minimum_required(VERSION 3.25)
project(img_to_ascii)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optionen für manuelle Pfade (können per -D übergeben werden)
set(FFMPEG_ROOT "" CACHE PATH "Root directory of FFmpeg installation")
set(FFMPEG_INCLUDE_DIRS "" CACHE PATH "FFmpeg include directories")
set(FFMPEG_LIBRARY_DIRS "" CACHE PATH "FFmpeg library directories")

# Versuche zuerst pkg-config
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    message(STATUS "pkg-config found. Trying to find FFmpeg...")
    pkg_check_modules(FFMPEG
            libavformat libavcodec libavutil libswscale libswresample
    )
endif()

if (FFMPEG_FOUND)
    message(STATUS "FFmpeg found via pkg-config")
endif()

# Fallback: manuelle Suche, falls pkg-config fehlschlägt
if(NOT FFMPEG_FOUND)
    message(STATUS "pkg-config not found or FFmpeg not registered. Trying manual detection...")

    # Wenn FFMPEG_ROOT gesetzt ist, leite die Pfade ab
    if(FFMPEG_ROOT)
        set(FFMPEG_INCLUDE_DIRS "${FFMPEG_ROOT}/include")
        set(FFMPEG_LIBRARY_DIRS "${FFMPEG_ROOT}/lib")
    endif()

    # Header-Datei suchen (stellvertretend für alle Bibliotheken)
    find_path(AVCODEC_INCLUDE_DIR libavcodec/avcodec.h
            HINTS ${FFMPEG_INCLUDE_DIRS}
            PATH_SUFFIXES ffmpeg
    )
    find_path(AVFORMAT_INCLUDE_DIR libavformat/avformat.h
            HINTS ${FFMPEG_INCLUDE_DIRS}
            PATH_SUFFIXES ffmpeg
    )
    # ... analog für avutil, swscale, swresample
    # Vereinfacht: wir nehmen an, dass alle im gleichen Include-Verzeichnis liegen.
    set(FFMPEG_INCLUDE_DIRS ${AVCODEC_INCLUDE_DIR})

    # Bibliotheken suchen (Windows: .lib oder .dll.a; Linux: .so; macOS: .dylib)
    find_library(AVCODEC_LIBRARY avcodec
            HINTS ${FFMPEG_LIBRARY_DIRS}
    )
    find_library(AVFORMAT_LIBRARY avformat
            HINTS ${FFMPEG_LIBRARY_DIRS}
    )
    find_library(AVUTIL_LIBRARY avutil
            HINTS ${FFMPEG_LIBRARY_DIRS}
    )
    find_library(SWSCALE_LIBRARY swscale
            HINTS ${FFMPEG_LIBRARY_DIRS}
    )
    find_library(SWRESAMPLE_LIBRARY swresample
            HINTS ${FFMPEG_LIBRARY_DIRS}
    )

    # Prüfen, ob alles gefunden wurde
    if(AVCODEC_INCLUDE_DIR AND AVCODEC_LIBRARY AND AVFORMAT_LIBRARY AND
            AVUTIL_LIBRARY AND SWSCALE_LIBRARY AND SWRESAMPLE_LIBRARY)
        set(FFMPEG_FOUND TRUE)
        set(FFMPEG_LIBRARIES
                ${AVCODEC_LIBRARY}
                ${AVFORMAT_LIBRARY}
                ${AVUTIL_LIBRARY}
                ${SWSCALE_LIBRARY}
                ${SWRESAMPLE_LIBRARY}
        )
        set(FFMPEG_INCLUDE_DIRS ${AVCODEC_INCLUDE_DIR})
        # Windows benötigt zusätzlich oft ws2_32, secur32, bcrypt etc.
        if(WIN32)
            list(APPEND FFMPEG_LIBRARIES ws2_32 secur32 bcrypt)
        endif()
        # pkg-config-ähnliche Compiler-Flags (können leer bleiben)
        set(FFMPEG_CFLAGS_OTHER "")
        message(STATUS "FFmpeg found manually: ${FFMPEG_LIBRARIES}")
    else()
        message(FATAL_ERROR "FFmpeg not found. Make sure it's installed and set FFMPEG_ROOT or provide paths manually.")
    endif()
endif()

if(WIN32 AND FFMPEG_LIBRARIES)
    # Annahme: FFMPEG_ROOT/bin enthält die DLLs
    file(GLOB FFMPEG_DLLS "${FFMPEG_ROOT}/bin/*.dll")
    foreach(DLL ${FFMPEG_DLLS})
        add_custom_command(TARGET img_to_ascii POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${DLL}" $<TARGET_FILE_DIR:img_to_ascii>
        )
    endforeach()
endif()

add_executable(img_to_ascii
        src/main.cpp
        src/Renderer.cpp
        src/GenerateFrames.cpp
        src/miniaudio.c
        src/utf8_stuff.cpp
        src/AudioExtractor.cpp
        src/AudioPlayer.cpp
        src/OutputWriter.cpp
        include/OutputWriter.hpp
)

target_include_directories(img_to_ascii PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${FFMPEG_INCLUDE_DIRS}
)

target_link_libraries(img_to_ascii PRIVATE ${FFMPEG_LIBRARIES})

target_compile_options(img_to_ascii PRIVATE ${FFMPEG_CFLAGS_OTHER})

# Warnstufe auf maximal und Warnungen als Fehler
set_target_properties(img_to_ascii PROPERTIES
        COMPILE_WARNING_AS_ERROR ON
)

if(MSVC)
    target_compile_options(img_to_ascii PRIVATE /W4)
else()
    target_compile_options(img_to_ascii PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # For your own good just activate one of the two debug modes, not both. you dont want the see the flickering
    target_compile_definitions(img_to_ascii PRIVATE DEBUG_MODE)
    # target_compile_definitions(img_to_ascii PRIVATE DEBUG_RENDERER_MODE)
else()
    target_compile_definitions(img_to_ascii PRIVATE NDEBUG)
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -g")
endif()