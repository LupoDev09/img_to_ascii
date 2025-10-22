//
// Created by lupo on 18.10.25.
//

// #define STB_IMAGE_WRITE_IMPLEMENTATION  // Implementation wird in
// stb_impl.cpp bereitgestellt
#include "../include/cxxopts.hpp"
#include "../include/gif_utils.h"
#include "../include/img_utils.h"
#include "../include/nlohmann/json.hpp"
#include "../include/verbose.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>

using namespace std;
namespace fs = std::filesystem;
using json = nlohmann::json;

/**
 * @brief Konfigurationsstruktur für die Anwendung
 */
struct Config {
  std::string ascii_chars = "@%#*+=-:. "; // Standard-Zeichensatz
  std::string tmp_frames_naming_scheme =
      "frame_%03d.png";             // Benennungsschema für temporäre GIF-Frames
  std::filesystem::path image_path; // Pfad zum Eingabebild
  std::filesystem::path
      output_path;               // Ausgabe-Dateipfad wenn man --output benutzt
  std::filesystem::path tmp_dir; // temporäres Verzeichnis für GIF-Frames
  std::filesystem::path load_config; // Lade eine config
  int width = 70;                    // Standardbreite ist 70 Zeichen
  int fps = 10;                      // Standard Frame-Rate für GIFs
  int loop = 0;                      // Standardmäßig 0 Loop (einmalige ausgabe)
  bool colored = false;              // standardmäßig keine farbige Ausgabe
  bool gif = false; // standardmäßig wird nicht davon ausgegangen das der input
                    // ein GIF ist
  bool keep_frames = false; // behalte temporäre Frames standardmäßig nicht
  bool write_json = false;  // schreibe methadaten in json
};

void overwrite_cfg_with_json_conf(Config &cfg, const std::string &json_path);

/**
 * @brief Setzt die Kommandozeilenoptionen mit cxxopts
 *
 * @return cxxopts::Options Objekt mit den definierten Optionen
 */
cxxopts::Options setup_options() {
  cxxopts::Options options("img_to_ascii",
                           "Konvertiert Bilder oder GIFs zu ASCII-Art");

  options.add_options()("h,help", "Zeigt diese Hilfe an")(
      "img", "Pfad zum Eingabebild oder GIF", cxxopts::value<std::string>())(
      "w,width", "Breite der ASCII-Ausgabe (Standard: 70)",
      cxxopts::value<int>())("fps",
                             "Frame-Rate für GIF-Animation (Standard: 10 FPS)",
                             cxxopts::value<int>())(
      "loop", "Anzahl der Wiederholungen (Standard: 0 = einmalig)",
      cxxopts::value<int>())("ascii",
                             "ASCII-Zeichensatz (Standard: '@%#*+=-:. ')",
                             cxxopts::value<std::string>())(
      "o,output", "Pfad zur Ausgabedatei", cxxopts::value<std::string>())(
      "colored", "Aktiviere farbige ASCII-Ausgabe",
      cxxopts::value<bool>()->default_value("false"))(
      "gif", "Behandle Eingabe als GIF oder Video",
      cxxopts::value<bool>()->default_value("false"))(
      "keep-frames", "Behalte temporäre Frames",
      cxxopts::value<bool>()->default_value("false"))(
      "tmp-dir", "Temporäres Verzeichnis für GIF-Frames",
      cxxopts::value<std::string>())("tmp-frames-naming-scheme",
                                     "Benennungsschema für GIF-Frames",
                                     cxxopts::value<std::string>())(
      "verbose, v", "Aktiviere Verbose modus",
      cxxopts::value<bool>()->default_value("false"))(
      "write-json", "schreibe meta daten in json",
      cxxopts::value<bool>()->default_value("false"))(
      "load-config", "lade eine config",
      cxxopts::value<std::string>()->default_value(""));
  return options;
}

/**
 * @brief Gibt die Hilfe auf der Konsole aus
 * @param options cxxopts::Options Objekt mit den definierten Optionen
 */
void print_help(const cxxopts::Options &options) {
  // Grundlegende Verwendung
  std::cout << "Usage:\n"
            << "  img_to_ascii --img <Pfad_zum_Bild> [Optionen]\n\n";

  // Automatisch generierter Hilfetext von cxxopts
  std::cout << options.help() << "\n";

  // Zusätzliche Hinweise
  std::cout << "Hinweise:\n"
            << "  • Wenn kein Bildpfad angegeben wird, wird "
               "'Silly_Cat_Character_.jpg' verwendet.\n"
            << "  • ANSI-Farben funktionieren nur in Terminals, die "
               "Escape-Sequenzen verstehen.\n"
            << "  • Es ist basically Glücksspiel, ob das Ding auf Windows "
               "läuft - viel Glück :3\n"
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
Config parse_args(const int argc, char *argv[]) {
  auto options = setup_options();                // Optionen einrichten
  const auto result = options.parse(argc, argv); // Argumente parsen

  if (result.count("help")) {
    // Hilfe anzeigen
    print_help(options);
    std::exit(0); // Program beenden nach Anzeige der Hilfe
  }

  Config cfg; // Konfigurationsstruktur initialisieren
  if (result.count("load-config"))
    cfg.load_config = result["load-config"].as<std::string>();
  if (cfg.load_config != "")
    overwrite_cfg_with_json_conf(cfg, cfg.load_config.string());

  // Fülle die Konfigurationsstruktur basierend auf den geparsten Argumenten
  // Int Werte
  if (result.count("width"))
    cfg.width = result["width"].as<int>();
  if (result.count("fps"))
    cfg.fps = result["fps"].as<int>();
  if (result.count("loop"))
    cfg.loop = result["loop"].as<int>();

  // String Werte
  if (result.count("ascii"))
    cfg.ascii_chars = result["ascii"].as<std::string>();
  if (result.count("output"))
    cfg.output_path = result["output"].as<std::string>();
  if (result.count("img"))
    cfg.image_path = result["img"].as<std::string>();
  if (result.count("tmp-dir"))
    cfg.tmp_dir = result["tmp-dir"].as<std::string>();
  if (result.count("tmp-frames-naming-scheme"))
    cfg.tmp_frames_naming_scheme =
        result["tmp-frames-naming-scheme"].as<std::string>();

  // Boolesche Flags
  if (result.count("colored"))
    cfg.colored = result["colored"].as<bool>();
  if (result.count("gif"))
    cfg.gif = result["gif"].as<bool>();
  if (result.count("keep-frames"))
    cfg.keep_frames = result["keep-frames"].as<bool>();
  if (result.count("write-json"))
    cfg.write_json = result["write-json"].as<bool>();
  VERBOSE_MODE = result["verbose"].as<bool>();

  verbose("Parsed Flags");
  return cfg;
}

/**
 * @brief Setzt Standardwerte für die Konfigurationsstruktur
 *
 * @param cfg Referenz auf die Konfigurationsstruktur
 */
void set_defaults(Config &cfg) {
  if (cfg.tmp_dir.empty())
    cfg.tmp_dir = "./tmp_gif_frames";
  verbose("set tmp_dir default");
  if (cfg.image_path.empty()) {
    verbose("set image_path default");
    const fs::path exe_dir = fs::current_path();
    cfg.image_path = exe_dir / "Silly_Cat_Character_.jpg";
    cout << "Kein Bildpfad angegeben, verwende Standardbild: " << cfg.image_path
         << endl;
  }
  verbose("set defaults");
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

  if (cfg.fps != 10 && !cfg.gif)
    throw std::runtime_error("--fps funktioniert nur mit --gif");

  if (cfg.width <= 0)
    throw std::runtime_error("Breite muss größer als 0 sein");

  if (cfg.width >= 1000)
    throw std::runtime_error(
        "Breite zu groß! Bitte einen Wert unter 1000 wählen.");

  if (cfg.width > 500)
    std::cout << "Warnung: Eine sehr große Breite kann die Anzeige in der "
                 "Konsole beeinträchtigen.\n";

  if (cfg.loop < 0)
    throw std::runtime_error("Loop-Wert muss größer oder gleich 0 sein");

  if (!fs::exists(cfg.image_path))
    throw std::runtime_error("Bild nicht gefunden: " + cfg.image_path.string());

  if (!fs::exists(cfg.load_config) && cfg.load_config != "")
    throw std::runtime_error("Config nicht gefunden: " +
                             cfg.load_config.string());

  // tmp_frames_naming_scheme Checks
  if (const std::regex re("%0?\\d*d");
      !std::regex_search(cfg.tmp_frames_naming_scheme, re))
    throw std::runtime_error(
        "--tmp_frames_naming_scheme muss ein '%d'-Platzhalter enthalten");
  if (cfg.tmp_frames_naming_scheme.find(".png") == std::string::npos &&
      cfg.tmp_frames_naming_scheme.find(".jpg") == std::string::npos &&
      cfg.tmp_frames_naming_scheme.find(".jpeg") == std::string::npos)
    throw std::runtime_error(
        "--tmp_frames_naming_scheme muss auf .png, .jpg oder .jpeg enden");
  verbose("Validated config");
}

/**
 * @brief Rendert das Bild in ASCII-Art basierend auf der Konfiguration
 *
 * @param cfg Referenz auf die Konfigurationsstruktur
 *
 * @return ASCII-Art als String or in the case of GIFs an empty string because
 * the output is handled directly
 */
std::string render_ascii(const Config &cfg) {
  std::string ascii = "";
  int loops = cfg.loop;
  cout << "Generiere ASCII-Art...\n";
  do {
    // GIF-Verarbeitung
    if (cfg.gif) {
      // GIF wird direkt ausgegeben, kein Rückgabewert
      gif_to_ascii(cfg.image_path.string(), cfg.width, cfg.ascii_chars,
                   cfg.keep_frames, cfg.colored, cfg.tmp_dir,
                   cfg.tmp_frames_naming_scheme, cfg.fps, cfg.write_json);
    }
    // Einzelbild-mit-Farbe-Verarbeitung
    else if (cfg.colored) {
      ascii = image_to_ascii_color(cfg.image_path.string(), cfg.width,
                                   cfg.ascii_chars);
    }
    // Normale Einzelbild-Verarbeitung
    else {
      ascii =
          image_to_ascii(cfg.image_path.string(), cfg.width, cfg.ascii_chars);
    }
  } while (loops-- > 0);
  verbose("Rendered ASCII");
  return ascii;
}

/**
 * @brief Gibt die ASCII-Art entweder auf der Konsole aus oder schreibt sie in
 * eine Datei
 *
 * @param ascii ASCII-Art als String
 * @param cfg Referenz auf die Konfigurationsstruktur
 */
void output_ascii(const std::string &ascii, const Config &cfg) {
  if (cfg.output_path.empty()) {
    std::cout << "\033[?25l";        // Verstecke den Cursor
    std::cout << ascii << std::endl; // Ausgabe auf der Konsole
    std::cout << "\033[?25h";        // Zeige den Cursor wieder
    verbose("Wrote ASCII to console");
  } else {
    std::ofstream out(cfg.output_path); // Ausgabe in Datei

    // Fehler beim Öffnen der Datei
    if (!out)
      throw std::runtime_error("Konnte Datei nicht öffnen: " +
                               cfg.output_path.string());

    // Schreibe ASCII-Art in die Datei
    out << ascii;
    std::cout << "ASCII-Art in Datei geschrieben: " << cfg.output_path
              << std::endl;
    verbose("Wrote ASCII to file");
  }
}

void overwrite_cfg_with_json_conf(Config &cfg, const std::string &json_path) {
  std::ifstream file(json_path);
  if (!file.is_open()) {
    std::cerr << "Fehler: Konnte " << json_path << " nicht öffnen!\n";
    return;
  }

  json j;
  try {
    file >> j;
  } catch (const std::exception &e) {
    std::cerr << "Fehler beim Einlesen der JSON-Datei: " << e.what() << "\n";
    return;
  }

  // Werte prüfen und überschreiben, falls sie existieren
  if (j.contains("ascii_chars"))
    cfg.ascii_chars = j["ascii_chars"].get<std::string>();
  if (j.contains("tmp_frames_naming_scheme"))
    cfg.tmp_frames_naming_scheme =
        j["tmp_frames_naming_scheme"].get<std::string>();
  if (j.contains("image_path"))
    cfg.image_path = j["image_path"].get<std::string>();
  if (j.contains("output_path"))
    cfg.output_path = j["output_path"].get<std::string>();
  if (j.contains("tmp_dir"))
    cfg.tmp_dir = j["tmp_dir"].get<std::string>();
  if (j.contains("width"))
    cfg.width = j["width"].get<int>();
  if (j.contains("fps"))
    cfg.fps = j["fps"].get<int>();
  if (j.contains("loop"))
    cfg.loop = j["loop"].get<int>();
  if (j.contains("colored"))
    cfg.colored = j["colored"].get<bool>();
  if (j.contains("gif"))
    cfg.gif = j["gif"].get<bool>();
  if (j.contains("keep_frames"))
    cfg.keep_frames = j["keep_frames"].get<bool>();
  if (j.contains("write_json"))
    cfg.write_json = j["write_json"].get<bool>();
}
#if defined(_WIN32)
#define NOMINMAX
#define byte win_byte_override
#include <windows.h>
#undef byte
#include <fcntl.h>
#include <io.h>

/**
 * @brief Enables Virtual Terminal (ANSI Escape) handling on Windows.
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
#endif