#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

// stb_image (Header-Only Image Loader) → https://github.com/nothings/stb
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#if defined(_WIN32)
// Special shit for windows because without this shit it won't work :3
#include <windows.h>
#include <io.h>
#include <fcntl.h>

void enable_vt_mode() {
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
#endif

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
string image_to_ascii(const string &filename, int output_width = 70, const string &ascii_chars = "@%#*+=-:. ") {
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
  int output_height = static_cast<int>(output_width * aspect_ratio * 0.55f);
  // 0.55 für Konsolen-Zeichenhöhe korrigiert

  string ascii;
  ascii.reserve(output_width * output_height + output_height);

  // Schrittgrößen fürs Sampling (Skalierung)
  float x_step = static_cast<float>(width) / output_width;
  float y_step = static_cast<float>(height) / output_height;

  for (int y = 0; y < output_height; y++) {
    for (int x = 0; x < output_width; x++) {
      int px = static_cast<int>(x * x_step);
      int py = static_cast<int>(y * y_step);

      int idx = (py * width + px) * channels;

      // RGB-Werte holen
      unsigned char r = img[idx + 0];
      unsigned char g = img[idx + 1];
      unsigned char b = img[idx + 2];

      // Transparenz prüfen, falls Alpha-Kanal vorhanden
      if (channels >= 4) {
        unsigned char a = img[idx + 3];
        if (a < 128) { // Pixel halbtransparent oder unsichtbar → Leerzeichen
          ascii.push_back(' ');
          continue;
        }
      }

      // Grauwert berechnen (RGB → Luminanz)
      auto gray = static_cast<unsigned char>(0.299 * r + 0.587 * g + 0.114 * b);

      // Mapping: Grau 0-255 auf ascii_chars
      int char_index = gray * (ascii_chars.size() - 1) / 255;
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
string image_to_ascii_color(const string &filename, int output_width = 70, const string &ascii_chars = "@%#*+=-:. ") {
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
  int output_height = static_cast<int>(output_width * aspect_ratio * 0.55f);

  string ascii;
  ascii.reserve(output_width * output_height * 30); // Platz für ANSI-Codes

  float x_step = static_cast<float>(width) / output_width;
  float y_step = static_cast<float>(height) / output_height;

  for (int y = 0; y < output_height; y++) {
    for (int x = 0; x < output_width; x++) {
      int px = static_cast<int>(x * x_step);
      int py = static_cast<int>(y * y_step);

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
      int char_index = gray * (ascii_chars.size() - 1) / 255;
      char c = ascii_chars[char_index];

      // ANSI 24-Bit Farbcodes (Vordergrundfarbe)
      ascii += "\033[38;2;" + to_string(r) + ";" + to_string(g) + ";" +
               to_string(b) + "m";
      ascii.push_back(c);
    }
    ascii += "\033[0m\n"; // Reset am Zeilenende
  }

  stbi_image_free(img);
  return ascii;
}

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
         << "  Es ist basically Gluekspiel ob das Ding auf Windows Lauft" << endl;
}

int main(int argc, char *argv[]) {
    try {
#if defined(_WIN32)
        // Damit UTF-8 und ANSI Farben in der Windows-Konsole funktionieren:
        _setmode(_fileno(stdout), _O_TEXT);
        enable_vt_mode();
#endif
        string ascii_chars = "@%#*+=-:. ";
        fs::path image_path;
        fs::path output_path;
        int width = 70;// Default
        bool colored = false;

        // Parse flags
        for (int i = 1; i < argc; i++) {
            string arg = argv[i];
            if ((arg == "-w" || arg == "--width") && i + 1 < argc) {
                // Get width parameter
                try {
                    width = stoi(argv[++i]);
                } catch (...) {
                    throw runtime_error("Ungültige Breite bei -w");
                }
            } else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
                output_path = fs::path(argv[++i]);
            } else if (arg == "--ascii" && i + 1 < argc) {
                ascii_chars = argv[++i];
            } else if (arg == "--colored") {
                colored = true;
            } else if (arg == "-h" || arg == "--help") {
                print_help();
                return 0;
            } else {// Get img path
                image_path = arg;
            }
        }

        if (colored && !output_path.empty()) {
            throw runtime_error("--colored und --output sind nicht kompatibel "
                                "(Farben brauchen Terminal)");
        }

        // if path is not provided use default path
        if (image_path.empty()) {
            fs::path exe_path = fs::absolute(argv[0]);
            fs::path exe_dir = exe_path.parent_path();
            image_path = exe_dir / "Silly_Cat_Character_.jpg";
        }

        // if ascii_chars is provided but is empty throw an error
        if (ascii_chars.empty()) {
            throw runtime_error("--ascii darf nicht leer sein! entweder lass es weg "
                                "oder gib es einen wert");
        }

        cout << "Lade: " << image_path << " (Breite: " << width << ")" << endl;
        string ascii;
        if (colored) {
            ascii = image_to_ascii_color(image_path.string(), width, ascii_chars);
        } else {
            ascii = image_to_ascii(image_path.string(), width, ascii_chars);
        }

        if (output_path.empty()) {
            cout << ascii << endl;
        } else {
            ofstream out(output_path);
            if (!out) {
                throw runtime_error("Konnte Datei nicht zum Schreiben öffnen: " +
                                    output_path.string());
            }
            out << ascii;
            cout << "ASCII-Art in Datei geschrieben: " << output_path << endl;
        }

    } catch (const exception &error) {
        cerr << error.what() << "\n";
        print_help();
        cout << "\033[0m" << endl;// Reset ganz am Ende von main
        return 1;
    }
    cout << "\033[0m" << endl; // Reset ganz am Ende von main
    return 0;
}
