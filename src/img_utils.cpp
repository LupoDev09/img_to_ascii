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
 * @brief Gibt die Hilfe auf der Konsole aus
 */
void print_help() {
    cout << "Usage:\n"
         << "  img_to_ascii --img <Path_to_img> [options]\n\n"
         << "Options:\n"
         << "  -h, --help                           Zeigt diese Hilfe an\n"
         << "  --img PATH                           Pfad zum Eingabebild (Standard: './Silly_Cat_Character_.jpg')\n"
         << "  -w, --width N                        Breite der ASCII-Ausgabe (Standard: 70)\n"
         << "  --ascii CHARS                        Zeichensatz für Helligkeit (Standard: "
            "\"@%#*+=-:. \")\n"
         << "  -o, --output PATH                    Ausgabe in Datei speichern\n"
         << "  --colored                            Farbausgabe im Terminal (nicht mit --output "
            "kombinierbar)\n"
         << "  --gif                                GIF-Datei als ASCII-Animation verarbeiten\n"
         << "  --keep-frames                        Temporäre extrahierte Frames bei GIF-Verarbeitung "
            "behalten\n"
         << "  --tmp_dir PATH                       Verzeichnis für temporäre GIF-Frames (Standard: "
            "'./tmp_gif_frames')\n"
         << "  --tmp_frames_naming_scheme SCHEME    Benennungsschema für temporäre GIF-Frames (derzeit nicht implementiert)\n"
         << "\n"
         << "\n"
         << "Hinweis:\n"
         << "  Wenn kein Bildpfad angegeben wird, wird "
            "'Silly_Cat_Character_.jpg' verwendet.\n"
         << "  Für die GIF-Verarbeitung wird ImageMagick benötigt.\n"
         << "  ANSI-Farben werden nur in Terminals unterstützt, die "
            "ANSI-Escape-Sequenzen verstehen.\n"
         << "  Es ist basically Glücksspiel ob das Ding auf Windows Lauft viel glueck :3" << endl;
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


#if defined(_WIN32)
// Special shit for windows because without this shit it won't work :3
#include <windows.h>
#include <io.h>
#include <fcntl.h>

/**
 * @brief If compiled on windows this funktion activates ansi-escape sequences in cmd for windows
 * on other OS's it writes a warning to cerr
 */
void enable_vt_mode() {
    _setmode(_fileno(stdout), _O_TEXT);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE)
        return;
    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode))
        return;
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(hOut, dwMode)) {
        std::cerr << "Warnung: ANSI-Farben werden eventuell nicht unterstützt.\n";
    }
}
#else
void enable_vt_mode() {
    cerr << "Ether you are not on Windows or something went wrong while compiling" << endl;
}
#endif
