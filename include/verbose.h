//
// Created by lupo on 19.10.25.
//

#ifndef IMG_TO_ASCII_VERBOSE_H
#define IMG_TO_ASCII_VERBOSE_H

#pragma once
#include <iostream>

namespace img_to_ascii{
    // Deklaration, alle Dateien können darauf zugreifen
    inline bool VERBOSE_MODE = false;

    // Hilfsfunktion für verbose-Ausgaben
    inline void verbose(const std::string &msg) {
        if (VERBOSE_MODE) {
            std::clog << "[verbose] " << msg << "\n";
        }
    }
}

#endif// IMG_TO_ASCII_VERBOSE_H
