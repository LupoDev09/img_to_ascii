//
// Created by lupo on 12.07.26.
//

#ifndef IMG_TO_ASCII_VERBOSE_HPP
#define IMG_TO_ASCII_VERBOSE_HPP
#include <iostream>
#include <string_view>

// Debug-Makro: Nur aktiv, wenn NDEBUG NICHT definiert ist (z. B. in Debug-Builds)
#ifdef NDEBUG
#define DEBUG(message) ((void) 0)// Wird zu nichts kompiliert
#else
#define DEBUG(message)                                                                                                 \
    do {                                                                                                               \
        std::cerr << "[DEBUG] " << __FILE__ << ":" << __LINE__ << " (" << __func__ << "): " << message << "\n";        \
    } while (0)
#endif

#ifndef NDEBUG
#include <thread>

#if defined(__linux__)
    #include <pthread.h>
    #define SET_THREAD_NAME(thread, name) \
        pthread_setname_np((thread).native_handle(), name)

#elif defined(__APPLE__)
    #include <pthread.h>
    // macOS: kann NUR den eigenen (aktuellen) Thread benennen, kein native_handle-Setter
    #define SET_THREAD_NAME(thread, name) \
        pthread_setname_np(name)

#elif defined(_WIN32)
    #include <windows.h>
    #include <processthreadsapi.h>
    #include <string>
    inline void set_thread_name_win(std::thread& t, const char* name) {
        std::wstring wname(name, name + strlen(name));
        SetThreadDescription(t.native_handle(), wname.c_str());
    }
#define SET_THREAD_NAME(thread, name) set_thread_name_win(thread, name)

#else
#define SET_THREAD_NAME(thread, name) ((void)0)
#endif

#else
#define SET_THREAD_NAME(thread, name) ((void)0)
#endif

#endif// IMG_TO_ASCII_VERBOSE_HPP
