//
// Created by lupo on 18.10.25.
//

//#define STB_IMAGE_WRITE_IMPLEMENTATION  // Implementation wird in stb_impl.cpp bereitgestellt
#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include "../include/img_utils.h"
#include "../include/gif_utils.h"

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

        << "  --loop N                             Anzahl der Wiederholungen der Ausgabe (Standard: 0 = einmalig)\n"

        << "  --ascii CHARS                        Zeichensatz für Helligkeit (Standard: "
            "\"@%#*+=-:. \")\n"

        << "  -o, --output PATH                    Ausgabe in Datei speichern\n"

        << "  --colored                            Farbausgabe im Terminal (nicht mit --output "
            "kombinierbar)\n"

        << "  --gif                                GIF-Datei als ASCII-Animation verarbeiten\n"
        << "                                         (benötigt ImageMagick) und unterstuetzt alles an Video vormaten was ImageMagick kann\n"

        << "  --fps N                              Frame-Rate für GIF-Animation (Standard: 1 FPS)\n"

        << "  --keep-frames                        Temporäre extrahierte Frames bei GIF-Verarbeitung "
            "behalten\n"

        << "  --tmp_dir PATH                       Verzeichnis für temporäre GIF-Frames (Standard: "
            "'./tmp_gif_frames')\n"

        << "  --tmp_frames_naming_scheme SCHEME    Benennungsschema für temporäre GIF-Frames (derzeit nicht implementiert)\n"

         << "\n"
         << "\n"
         << "Hinweise:\n"
         << "  Wenn kein Bildpfad angegeben wird, wird "
            "'Silly_Cat_Character_.jpg' verwendet.\n"
         << "  Für die GIF-Verarbeitung wird ImageMagick benötigt.\n"
         << "  ANSI-Farben werden nur in Terminals unterstützt, die "
            "ANSI-Escape-Sequenzen verstehen.\n"
         << "  Es ist basically Glücksspiel ob das Ding auf Windows Lauft viel glueck :3" << endl;
}

/**
 * @brief Konfigurationsstruktur für die Anwendung
 */
struct Config {
    string ascii_chars = "@%#*+=-:. ";                      // Standard-Zeichensatz
    string tmp_frames_naming_scheme = "frame_%03d.png";     // Benennungsschema für temporäre GIF-Frames
    std::filesystem::path image_path;                       // Pfad zum Eingabebild
    std::filesystem::path output_path;                      // Ausgabe-Dateipfad wenn man --output benutzt
    std::filesystem::path tmp_dir;                          // temporäres Verzeichnis für GIF-Frames
    int width = 70;                                         // Standardbreite ist 70 Zeichen
    int fps = 1;                                            // Standard Frame-Rate für GIFs
    int loop = 0;                                           // Standardmäßig 0 Loop (einmalige ausgabe)
    bool colored = false;                                   // standardmäßig keine farbige Ausgabe
    bool gif = false;                                       // standardmäßig wird nicht davon ausgegangen das der input ein GIF ist
    bool keep_frames = false;                               // behalte temporäre Frames standardmäßig nicht
};

/**
 * @brief Parst die Kommandozeilenargumente und füllt die Konfigurationsstruktur
 *
 * @param argc Anzahl der Argumente
 * @param argv Array der Argumente
 * @return Gefüllte Konfigurationsstruktur
 */
Config parse_args(int argc, char* argv[]) {
    Config cfg;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "-w" || arg == "--width") && i + 1 < argc) {
            cfg.width = std::stoi(argv[++i]);
        } else if (arg == "--fps" && i + 1 < argc) {
            cfg.fps = std::stoi(argv[++i]);
        } else if (arg == "--loop" && i + 1 < argc) {
            cfg.loop = std::stoi(argv[++i]);
        } else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
            cfg.output_path = argv[++i];
        } else if (arg == "--ascii" && i + 1 < argc) {
            cfg.ascii_chars = argv[++i];
        } else if (arg == "--colored") {
            cfg.colored = true;
        } else if (arg == "--gif") {
            cfg.gif = true;
        } else if (arg == "--keep-frames") {
            cfg.keep_frames = true;
        } else if (arg == "--tmp_dir" && i + 1 < argc) {
            cfg.tmp_dir = argv[++i];
        } else if (arg == "--tmp_frames_naming_scheme" && i + 1 < argc) {
            cfg.tmp_frames_naming_scheme = argv[++i];
        } else if (arg == "--img" && i + 1 < argc) {
            cfg.image_path = argv[++i];
        } else if (arg == "-h" || arg == "--help") {
            print_help();
            std::exit(0);
        } else {
            std::cerr << "Unbekanntes Argument: " << arg << "\n";
        }
    }

    return cfg;
}

/**
 * @brief Setzt Standardwerte für die Konfigurationsstruktur
 *
 * @param cfg Referenz auf die Konfigurationsstruktur
 * @param exe_path Pfad zur ausführbaren Datei
 */
void set_defaults(Config &cfg, const char* exe_path) {
    if (cfg.tmp_dir.empty()) cfg.tmp_dir = "./tmp_gif_frames";
    if (cfg.image_path.empty()) {
        fs::path exe_dir = fs::absolute(exe_path).parent_path();
        cfg.image_path = exe_dir / "Silly_Cat_Character_.jpg";
    }
}

/**
 * @brief Validiert die Konfigurationsstruktur
 *
 * @param cfg Referenz auf die Konfigurationsstruktur
 * @throws std::runtime_error Bei ungültiger Konfiguration
 */
void validate_config(const Config &cfg) {
    if (cfg.ascii_chars.empty())
        throw std::runtime_error("--ascii darf nicht leer sein!");

    if (cfg.colored && !cfg.output_path.empty())
        throw std::runtime_error("--colored und --output sind nicht kompatibel");

    if (cfg.keep_frames && !cfg.gif)
        throw std::runtime_error("--keep-frames funktioniert nur mit --gif");

    if (cfg.fps <= 0)
        throw std::runtime_error("Frame-Rate muss größer als 0 sein");

    if (cfg.fps != 1 && !cfg.gif)
        throw std::runtime_error("--fps funktioniert nur mit --gif");

    if (cfg.width <= 0) throw std::runtime_error("Breite muss größer als 0 sein");

    if (!fs::exists(cfg.image_path))
        throw std::runtime_error("Datei nicht gefunden: " + cfg.image_path.string());

    // tmp_frames_naming_scheme Checks
    std::regex re("%0?\\d*d");
    if (!std::regex_search(cfg.tmp_frames_naming_scheme, re))
        throw std::runtime_error("--tmp_frames_naming_scheme muss ein '%d'-Platzhalter enthalten");
    if (cfg.tmp_frames_naming_scheme.find(".png") == std::string::npos &&
        cfg.tmp_frames_naming_scheme.find(".jpg") == std::string::npos &&
        cfg.tmp_frames_naming_scheme.find(".jpeg") == std::string::npos)
        throw std::runtime_error("--tmp_frames_naming_scheme muss auf .png, .jpg oder .jpeg enden");
}

/**
 * @brief Rendert das Bild in ASCII-Art basierend auf der Konfiguration
 *
 * @param cfg Referenz auf die Konfigurationsstruktur
 * @return ASCII-Art als String or in the case of GIFs an empty string because the output is handled directly
 */
std::string render_ascii(const Config &cfg) {
    std::string ascii;
    int loops = cfg.loop;
    cout << "Generiere ASCII-Art...\n";
    do {
        if (cfg.gif) {
            gif_to_ascii(cfg.image_path.string(), cfg.width, cfg.ascii_chars,
                         cfg.keep_frames, cfg.colored, cfg.tmp_dir, cfg.tmp_frames_naming_scheme, cfg.fps);
        } else if (cfg.colored) {
            ascii = image_to_ascii_color(cfg.image_path.string(), cfg.width, cfg.ascii_chars);
        } else {
            ascii = image_to_ascii(cfg.image_path.string(), cfg.width, cfg.ascii_chars);
        }
    } while (loops-- > 0);
    return ascii;
}

/**
 * @brief Gibt die ASCII-Art entweder auf der Konsole aus oder schreibt sie in eine Datei
 *
 * @param ascii ASCII-Art als String
 * @param cfg Referenz auf die Konfigurationsstruktur
 */
void output_ascii(const std::string &ascii, const Config &cfg) {
    if (cfg.output_path.empty()) {
        std::cout << ascii << std::endl;
    } else {
        std::ofstream out(cfg.output_path);
        if (!out) throw std::runtime_error("Konnte Datei nicht öffnen: " + cfg.output_path.string());
        out << ascii;
        std::cout << "ASCII-Art in Datei geschrieben: " << cfg.output_path << std::endl;
    }
}
