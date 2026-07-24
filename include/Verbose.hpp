//
// Created by lupo on 12.07.26.
//

#ifndef IMG_TO_ASCII_VERBOSE_HPP
#define IMG_TO_ASCII_VERBOSE_HPP
#include <iostream>
#include <string_view>

// Debug-Makro: Nur aktiv, wenn NDEBUG NICHT definiert ist (z. B. in Debug-Builds)
#ifdef NDEBUG
    #define DEBUG(message) ((void)0)  // Wird zu nichts kompiliert
#else
    #define DEBUG(message) \
        do { \
            std::cerr << "[DEBUG] " << __FILE__ << ":" << __LINE__ << " (" << __func__ << "): " << message << "\n"; \
        } while (0)
#endif

#endif// IMG_TO_ASCII_VERBOSE_HPP
