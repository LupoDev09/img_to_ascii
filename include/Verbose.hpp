//
// Created by lupo on 12.07.26.
//

#ifndef IMG_TO_ASCII_VERBOSE_HPP
#define IMG_TO_ASCII_VERBOSE_HPP
#include <iostream>

/// Macro to output messages only when DEBUG_MODE is enabled
#ifdef DEBUG_MODE
#define DEBUG(msg) std::clog << (msg) << '\n'
#else
#define DEBUG(msg)
#endif

#endif// IMG_TO_ASCII_VERBOSE_HPP
