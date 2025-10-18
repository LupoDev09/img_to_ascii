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
#include "../include/cxxopts.hpp"

using namespace std;
namespace fs = std::filesystem;


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
 * @brief Setzt die Kommandozeilenoptionen mit cxxopts
 *
 * @return cxxopts::Options Objekt mit den definierten Optionen
 */
cxxopts::Options setup_options() {
    cxxopts::Options options("img_to_ascii", "Konvertiert Bilder oder GIFs zu ASCII-Art");

    options.add_options()
        ("h,help", "Zeigt diese Hilfe an")
        ("img", "Pfad zum Eingabebild oder GIF", cxxopts::value<std::string>())
        ("w,width", "Breite der ASCII-Ausgabe (Standard: 70)", cxxopts::value<int>())
        ("fps", "Frame-Rate für GIF-Animation (Standard: 1 FPS)", cxxopts::value<int>())
        ("loop", "Anzahl der Wiederholungen (Standard: 0 = einmalig)", cxxopts::value<int>())
        ("ascii", "ASCII-Zeichensatz (Standard: '@%#*+=-:. ')", cxxopts::value<std::string>())
        ("o,output", "Pfad zur Ausgabedatei", cxxopts::value<std::string>())
        ("colored", "Aktiviere farbige ASCII-Ausgabe", cxxopts::value<bool>()->default_value("false"))
        ("gif", "Behandle Eingabe als GIF oder Video (benötigt ImageMagick)", cxxopts::value<bool>()->default_value("false"))
        ("keep-frames", "Behalte temporäre Frames", cxxopts::value<bool>()->default_value("false"))
        ("tmp_dir", "Temporäres Verzeichnis für GIF-Frames", cxxopts::value<std::string>())
        ("tmp_frames_naming_scheme", "Benennungsschema für GIF-Frames", cxxopts::value<std::string>());

    return options;
}

/**
 * @brief Gibt die Hilfe auf der Konsole aus
 * @param options cxxopts::Options Objekt mit den definierten Optionen
 */
void print_help(const cxxopts::Options& options) {
    std::cout << "Usage:\n"
              << "  img_to_ascii --img <Pfad_zum_Bild> [Optionen]\n\n";

    // Automatisch generierter Hilfetext von cxxopts
    std::cout << options.help() << "\n";

    // Zusätzliche Hinweise
    std::cout << "Hinweise:\n"
              << "  • Wenn kein Bildpfad angegeben wird, wird 'Silly_Cat_Character_.jpg' verwendet.\n"
              << "  • Für GIF- oder Video-Verarbeitung wird ImageMagick benötigt.\n"
              << "  • ANSI-Farben funktionieren nur in Terminals, die Escape-Sequenzen verstehen.\n"
              << "  • Es ist basically Glücksspiel, ob das Ding auf Windows läuft - viel Glück :3\n"
              << std::endl;
}

/**
 * @brief Parst die Kommandozeilenargumente und füllt die Konfigurationsstruktur
 *
 * @param argc Anzahl der Argumente
 * @param argv Array der Argumente
 *
 * @return Gefüllte Konfigurationsstruktur
 */
Config parse_args(int argc, char* argv[]) {
    auto options = setup_options();
    auto result = options.parse(argc, argv);

    if (result.count("help")) {
        print_help(options);
        std::exit(0);
    }

    Config cfg;

    if (result.count("width")) cfg.width = result["width"].as<int>();
    if (result.count("fps")) cfg.fps = result["fps"].as<int>();
    if (result.count("loop")) cfg.loop = result["loop"].as<int>();
    if (result.count("ascii")) cfg.ascii_chars = result["ascii"].as<std::string>();
    if (result.count("output")) cfg.output_path = result["output"].as<std::string>();
    if (result.count("img")) cfg.image_path = result["img"].as<std::string>();
    if (result.count("tmp_dir")) cfg.tmp_dir = result["tmp_dir"].as<std::string>();
    if (result.count("tmp_frames_naming_scheme"))
        cfg.tmp_frames_naming_scheme = result["tmp_frames_naming_scheme"].as<std::string>();

    cfg.colored = result["colored"].as<bool>();
    cfg.gif = result["gif"].as<bool>();
    cfg.keep_frames = result["keep-frames"].as<bool>();

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
 *
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
 *
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
        std::cout << "\033[?25l";
        std::cout << ascii << std::endl;
        std::cout << "\033[?25h";
    } else {
        std::ofstream out(cfg.output_path);
        if (!out) throw std::runtime_error("Konnte Datei nicht öffnen: " + cfg.output_path.string());
        out << ascii;
        std::cout << "ASCII-Art in Datei geschrieben: " << cfg.output_path << std::endl;
    }
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