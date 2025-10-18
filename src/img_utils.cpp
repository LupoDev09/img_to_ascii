//
// Created by lupo on 17.10.25.
//



// stb_image headers (Implementierung befindet sich in stb_impl.cpp)
#include <string>
#include <filesystem>
#include <iostream>
#include "../include/img_utils.h"
#include "../include/stb_image.h"

using namespace std;
namespace fs = std::filesystem;

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
string image_to_ascii(const string &filename, const int output_width, const string &ascii_chars ) {
    int width, height, channels_in_file;
    constexpr int desired_channels = 4; // force RGBA so we always have at least RGB
    unsigned char *img =
            stbi_load(filename.c_str(), &width, &height, &channels_in_file, desired_channels);
    if (!img) {
        throw runtime_error("Fehler: Bild konnte nicht geladen werden!");
    }

    constexpr int used_channels = desired_channels; // buffer is returned with this many channels
    if (channels_in_file < 3) {
        // Warnung, aber nicht fatal: wir haben durch forced channels trotzdem RGB
        cerr << "Warnung: Quelldatei hat nur " << channels_in_file << " Kanäle; konvertiere zu RGB.\n";
    }

    // Zielhöhe proportional skalieren
    const float aspect_ratio = static_cast<float>(height) / static_cast<float>(width);
    const int output_height = static_cast<int>(static_cast<float>(output_width) * aspect_ratio * 0.55f);
    // 0.55 für Konsolen-Zeichenhöhe korrigiert

    string ascii;
    ascii.reserve(static_cast<unsigned long>(output_width * output_height + output_height));

    // Schrittgrößen fürs Sampling (Skalierung)
    const float x_step = static_cast<float>(width) / static_cast<float>(output_width);
    const float y_step = static_cast<float>(height) / static_cast<float>(output_height);

    for (int y = 0; y < output_height; y++) {
        for (int x = 0; x < output_width; x++) {
            const int px = static_cast<int>(static_cast<float>(x) * x_step);
            const int py = static_cast<int>(static_cast<float>(y) * y_step);

            const int idx = (py * width + px) * used_channels;

            // RGB-Werte holen
            const unsigned char r = img[idx + 0];
            const unsigned char g = img[idx + 1];
            const unsigned char b = img[idx + 2];

            // Transparenz prüfen, falls die Quelldatei tatsächlich ein Alpha-Kanal hat
            if (channels_in_file >= 4) {
                const unsigned char a = img[idx + 3];
                if (a < 128) { // Pixel halbtransparent oder unsichtbar → Leerzeichen
                    ascii.push_back(' ');
                    continue;
                }
            }

            // Grauwert berechnen (RGB → Luminanz)
            const unsigned char gray = static_cast<unsigned char>(0.299 * r + 0.587 * g + 0.114 * b);

            // Mapping: Grau 0-255 auf ascii_chars
            const unsigned long char_index = static_cast<unsigned long>(gray * static_cast<double>(ascii_chars.size() - 1) / 255.0);
            ascii.push_back(ascii_chars[char_index]);
        }
        ascii += '\n';
    }
    stbi_image_free(img);
  return ascii;
}

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
string image_to_ascii_color(const string &filename, const int output_width, const string &ascii_chars) {
    int width, height, channels_in_file;
    constexpr int desired_channels = 4; // force RGBA
    unsigned char *img =
            stbi_load(filename.c_str(), &width, &height, &channels_in_file, desired_channels);
    if (!img) {
        throw runtime_error("Fehler: Bild konnte nicht geladen werden!");
    }

    constexpr  int used_channels = desired_channels;
    if (channels_in_file < 3) {
        cerr << "Warnung: Quelldatei hat nur " << channels_in_file << " Kanäle; konvertiere zu RGB.\n";
    }

    const float aspect_ratio = static_cast<float>(height) / static_cast<float>(width);
    const int output_height = static_cast<int>(static_cast<float>(output_width) * aspect_ratio * 0.55f);

    string ascii;
    ascii.reserve(static_cast<unsigned long> (output_width * output_height * 30));// Platz für ANSI-Codes

    const float x_step = static_cast<float>(width) / static_cast<float>(output_width);
    const float y_step = static_cast<float>(height) / static_cast<float>(output_height);

    for (int y = 0; y < output_height; y++) {
        for (int x = 0; x < output_width; x++) {
            const int px = static_cast<int>(static_cast<float>(x) * x_step);
            const int py = static_cast<int>(static_cast<float>(y) * y_step);

            const int idx = (py * width + px) * used_channels;

            const unsigned char r = img[idx + 0];
            const unsigned char g = img[idx + 1];
            const unsigned char b = img[idx + 2];

            // Transparenz prüfen
            if (channels_in_file >= 4) {
                unsigned char a = img[idx + 3];
                if (a < 128) {
                    ascii += " ";
                    continue;
                }
            }

            // Grauwert → Zeichen auswählen
            const unsigned char gray = static_cast<unsigned char>(0.299 * r + 0.587 * g + 0.114 * b);
            const unsigned long char_index = static_cast<unsigned long>(gray * static_cast<double>(ascii_chars.size() - 1) / 255.0);
            const char c = ascii_chars[char_index];

            // ANSI 24-Bit Farbcodes (Vordergrundfarbe)
            ascii += "\033[38;2;" + to_string(r) + ";" + to_string(g) + ";" +
                     to_string(b) + "m";
            ascii.push_back(c);
        }
        ascii += "\033[0m\n";// Reset am Zeilenende
    }
    stbi_image_free(img);
    return ascii;
}



