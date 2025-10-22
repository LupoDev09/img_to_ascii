//
// Created by lupo on 18.10.25.
//

#ifndef IMG_TO_ASCII_GENERAL_UTILS_H
#define IMG_TO_ASCII_GENERAL_UTILS_H

#include "cxxopts.hpp"

/**
 * @brief Gibt die Hilfe auf der Konsole aus
 * @param options cxxopts::Options Objekt mit den definierten Optionen
 */
void print_help( const cxxopts::Options& options );

/**
 * @brief Konfigurationsstruktur für die Anwendung
 */
struct Config {
    std::string ascii_chars = "@%#*+=-:. "; // Standard-Zeichensatz
    std::string tmp_frames_naming_scheme = "frame_%03d.png"; // Benennungsschema für temporäre GIF-Frames
    std::filesystem::path image_path; // Pfad zum Eingabebild
    std::filesystem::path output_path; // Ausgabe-Dateipfad wenn man --output benutzt
    std::filesystem::path tmp_dir; // temporäres Verzeichnis für GIF-Frames
    std::filesystem::path load_config; // Lade eine config
    int width = 70; // Standardbreite ist 70 Zeichen
    int fps = 10; // Standard Frame-Rate für GIFs
    int loop = 0; // Standardmäßig 0 Loop (einmalige ausgabe)
    bool colored = false; // standardmäßig keine farbige Ausgabe
    bool gif = false; // standardmäßig wird nicht davon ausgegangen das der input ein GIF ist
    bool keep_frames = false; // behalte temporäre Frames standardmäßig nicht
};

/**
 * @brief Parst die Kommandozeilenargumente und füllt die Konfigurationsstruktur
 *
 * @param argc Anzahl der Argumente
 * @param argv Array der Argumente
 * @return Gefüllte Konfigurationsstruktur
 */
Config parse_args( int argc, char* argv[] );

/**
 * @brief Setzt Standardwerte für die Konfigurationsstruktur
 *
 * @param cfg Referenz auf die Konfigurationsstruktur
 * @param exe_path Pfad zur ausführbaren Datei
 */
void set_defaults( Config& cfg );

/**
 * @brief Validiert die Konfigurationsstruktur
 *
 * @param cfg Referenz auf die Konfigurationsstruktur
 * @throws std::runtime_error Bei ungültiger Konfiguration
 */
void validate_config( const Config& cfg );

/**
 * @brief Rendert das Bild in ASCII-Art basierend auf der Konfiguration
 *
 * @param cfg Referenz auf die Konfigurationsstruktur
 * @return ASCII-Art als String or in the case of GIFs an empty string because the output is handled directly
 */
std::string render_ascii( const Config& cfg );

/**
 * @brief Gibt die ASCII-Art entweder auf der Konsole aus oder schreibt sie in eine Datei
 *
 * @param ascii ASCII-Art als String
 * @param cfg Referenz auf die Konfigurationsstruktur
 */
void output_ascii( const std::string& ascii, const Config& cfg );

void overwrite_cfg_with_json_conf( Config& cfg, const std::string& json_path );

#if defined(_WIN32)
/**
 * @brief Enables Virtual Terminal (ANSI Escape) handling on Windows.
 */
void enable_vt_mode();
#endif

#endif //IMG_TO_ASCII_GENERAL_UTILS_H