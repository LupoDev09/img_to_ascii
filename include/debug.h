//
// Created by lupo on 19.10.25.
//

#ifndef IMG_TO_ASCII_DEBUG_H
#define IMG_TO_ASCII_DEBUG_H

#pragma once
#include <iostream>

// Deklaration, alle Dateien können darauf zugreifen
extern bool DEBUG_MODE;

// Hilfsfunktion für Debug-Ausgaben
inline void debug(const std::string &msg) {
    if (DEBUG_MODE) {
        std::cerr << "[DEBUG] " << msg << "\n";
    }
}


#endif //IMG_TO_ASCII_DEBUG_H
