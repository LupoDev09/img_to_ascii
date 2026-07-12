//
// Created by lupo on 12.07.26.
//

#ifndef IMG_TO_ASCII_VERBOSE_HPP
#define IMG_TO_ASCII_VERBOSE_HPP
#include <iostream>

/// Global flag to control verbose logging throughout the application
inline constexpr bool DEBUG_MODE = false;

/// Macro to output messages only when DEBUG_MODE is enabled
#define DEBUG(msg) if constexpr (DEBUG_MODE) std::clog << (msg) << '\n'

#endif// IMG_TO_ASCII_VERBOSE_HPP
