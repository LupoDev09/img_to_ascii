//
// Created by lupo on 17.10.25.
//

// stb_image (Header-Only Image Loader) → https://github.com/nothings/stb
#define STB_IMAGE_IMPLEMENTATION

#include <string>
#include <filesystem>
#include <iostream>
#include "utils.h"
// stb_image (Header-Only Image Loader) → https://github.com/nothings/stb Nicht Meins!!!
#include "stb_image.h"

using namespace std;
namespace fs = std::filesystem;

/**
 * @brief Gibt die Hilfe auf der Konsole aus
 */
void print_help() {
    cout << "Usage:\n"
         << "  img_to_ascii <Path_to_img> [options]\n\n"
         << "Options:\n"
         << "  -h, --help        Zeigt diese Hilfe an\n"
         << "  -w, --width N     Breite der ASCII-Ausgabe (Standard: 70)\n"
         << "  --ascii CHARS     Zeichensatz für Helligkeit (Standard: "
            "\"@%#*+=-:. \")\n"
         << "  -o, --output PATH Ausgabe in Datei speichern\n"
         << "  --colored         Farbausgabe im Terminal (nicht mit --output "
            "kombinierbar)\n\n"
         << "Hinweis:\n"
         << "  Wenn kein Bildpfad angegeben wird, wird "
            "'Silly_Cat_Character_.jpg' verwendet.\n"
         << "  Es ist basically Glücksspiel ob das Ding auf Windows Lauft :3" << endl;
}

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
string image_to_ascii(const string &filename, int output_width, const string &ascii_chars ) {
    int width, height, channels;
    unsigned char *img =
            stbi_load(filename.c_str(), &width, &height, &channels, 0);
    if (!img) {
        throw runtime_error("Fehler: Bild konnte nicht geladen werden!");
    }

    if (channels < 3) {
        stbi_image_free(img);
        throw runtime_error("Bild hat zu wenige Farbkanäle (mind. RGB nötig)");
    }

    // Zielhöhe proportional skalieren
    float aspect_ratio = static_cast<float>(height) / static_cast<float>(width);
    int output_height = static_cast<int>(static_cast<float>(output_width) * aspect_ratio * 0.55f);
    // 0.55 für Konsolen-Zeichenhöhe korrigiert

    string ascii;
    ascii.reserve(output_width * output_height + output_height);

    // Schrittgrößen fürs Sampling (Skalierung)
    float x_step = static_cast<float>(width) / static_cast<float>(output_width);
    float y_step = static_cast<float>(height) / static_cast<float>(output_height);

    for (int y = 0; y < output_height; y++) {
        for (int x = 0; x < output_width; x++) {
            int px = static_cast<int>(static_cast<float>(x) * x_step);
            int py = static_cast<int>(static_cast<float>(y) * y_step);

            int idx = (py * width + px) * channels;

            // RGB-Werte holen
            unsigned char r = img[idx + 0];
            unsigned char g = img[idx + 1];
            unsigned char b = img[idx + 2];

            // Transparenz prüfen, falls Alpha-Kanal vorhanden
            if (channels >= 4) {
                unsigned char a = img[idx + 3];
                if (a < 128) {// Pixel halbtransparent oder unsichtbar → Leerzeichen
                    ascii.push_back(' ');
                    continue;
                }
            }

            // Grauwert berechnen (RGB → Luminanz)
            auto gray = static_cast<unsigned char>(0.299 * r + 0.587 * g + 0.114 * b);

            // Mapping: Grau 0-255 auf ascii_chars
            int char_index = static_cast<int>(gray * static_cast<double>(ascii_chars.size() - 1) / 255.0);
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
string image_to_ascii_color(const string &filename, int output_width, const string &ascii_chars) {
    int width, height, channels;
    unsigned char *img =
            stbi_load(filename.c_str(), &width, &height, &channels, 0);
    if (!img) {
        throw runtime_error("Fehler: Bild konnte nicht geladen werden!");
    }

    if (channels < 3) {
        stbi_image_free(img);
        throw runtime_error("Bild hat zu wenige Farbkanäle (mind. RGB nötig)");
    }

    float aspect_ratio = static_cast<float>(height) / static_cast<float>(width);
    int output_height = static_cast<int>(static_cast<float>(output_width) * aspect_ratio * 0.55f);

    string ascii;
    ascii.reserve(output_width * output_height * 30);// Platz für ANSI-Codes

    float x_step = static_cast<float>(width) / static_cast<float>(output_width);
    float y_step = static_cast<float>(height) / static_cast<float>(output_height);

    for (int y = 0; y < output_height; y++) {
        for (int x = 0; x < output_width; x++) {
            int px = static_cast<int>(static_cast<float>(x) * x_step);
            int py = static_cast<int>(static_cast<float>(y) * y_step);

            int idx = (py * width + px) * channels;

            unsigned char r = img[idx + 0];
            unsigned char g = img[idx + 1];
            unsigned char b = img[idx + 2];

            // Transparenz prüfen
            if (channels >= 4) {
                unsigned char a = img[idx + 3];
                if (a < 128) {
                    ascii += " ";
                    continue;
                }
            }

            // Grauwert → Zeichen auswählen
            auto gray = static_cast<unsigned char>(0.299 * r + 0.587 * g + 0.114 * b);
            int char_index = static_cast<int>(gray * static_cast<double>(ascii_chars.size() - 1) / 255.0);
            char c = ascii_chars[char_index];

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
