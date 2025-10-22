//
// Created by lupo on 19.10.25.
//

#ifndef IMG_TO_ASCII_VERBOSE_H
#define IMG_TO_ASCII_VERBOSE_H

#pragma once
#include <iostream>

// Deklaration, alle Dateien können darauf zugreifen
extern bool VERBOSE_MODE;

// Hilfsfunktion für verbose-Ausgaben
inline void verbose( const std::string& msg ) { if ( VERBOSE_MODE ) { std::cerr << "[verbose] " << msg << "\n"; } }


#endif //IMG_TO_ASCII_VERBOSE_H