//
// Created by lupo on 17.10.25.
//

#ifndef IMG_TO_ASCII_UTILS_H
#define IMG_TO_ASCII_UTILS_H
#include <string>

/**
 * Wandelt ein Bild in ASCII-Art um.
 *
 * @brief converts a provided img to ascii art
 *
 * @param filename Pfad zur Bilddatei, die geladen werden soll.
 * @param output_width Breite der ASCII-Ausgabe in Zeichen (Standard: 70).
 * @param ascii_chars Zeichenfolge von dunkel → hell zur Darstellung (Standard: "\@%#*+=-:. ").
 *
 * @return ASCII-Art als String, Zeilen durch'\n' getrennt.
 *
 * @throws std::runtime_error Falls das Bild nicht geladen werden kann oder zu wenige Kanäle hat.
 */
std::string image_to_ascii(const std::string &filename,
    int output_width = 70,
    const std::string &ascii_chars = "@%#*+=-:. ");

/**
 * Wandelt ein Bild in ASCII-Art mit farbe um.
 *
 * @brief converts a provided img to ascii art
 *
 * @param filename Pfad zur Bilddatei, die geladen werden soll.
 * @param output_width Breite der ASCII-Ausgabe in Zeichen (Standard: 70).
 * @param ascii_chars Zeichenfolge von dunkel → hell zur Darstellung (Standard: "\@%#*+=-:. ").
 *
 * @return ASCII-Art als String, Zeilen durch'\n' getrennt.
 *
 * @throws std::runtime_error Falls das Bild nicht geladen werden kann oder zu wenige Kanäle hat.
 */
std::string image_to_ascii_color(const std::string &filename,
    int output_width = 70,
    const std::string &ascii_chars = "@%#*+=-:. ");

#endif //IMG_TO_ASCII_UTILS_H
