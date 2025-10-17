//
// Created by lupo on 17.10.25.
//

#ifndef IMG_TO_ASCII_UTILS_H
#define IMG_TO_ASCII_UTILS_H
#include <string>

void print_help();                                                 // Gibt die Hilfe auf der Konsole aus

std::string image_to_ascii(const std::string &filename,
    int output_width = 70,
    const std::string &ascii_chars = "@%#*+=-:. ");             // Wandelt ein Bild in ASCII-Art um.

std::string image_to_ascii_color(const std::string &filename,
    int output_width = 70,
    const std::string &ascii_chars = "@%#*+=-:. ");             // Wandelt ein Bild in ASCII-Art mit farbe um.

#endif //IMG_TO_ASCII_UTILS_H
