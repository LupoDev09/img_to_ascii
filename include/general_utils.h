//
// Created by lupo on 18.10.25.
//

#ifndef IMG_TO_ASCII_GENERAL_UTILS_H
#define IMG_TO_ASCII_GENERAL_UTILS_H

void print_help();


struct Config {
    std::string ascii_chars = "@%#*+=-:. ";                     // Standard-Zeichensatz
    std::string tmp_frames_naming_scheme = "frame_%03d.png";    // Benennungsschema für temporäre GIF-Frames
    std::filesystem::path image_path;                           // Pfad zum Eingabebild
    std::filesystem::path output_path;                          // Ausgabe-Dateipfad wenn man --output benutzt
    std::filesystem::path tmp_dir;                              // temporäres Verzeichnis für GIF-Frames
    int width = 70;                                             // Standardbreite ist 70 Zeichen
    int fps = 1;                                                // Standard Frame-Rate für GIFs
    int loop = 0;                                               // Standardmäßig 0 Loop (einmalige ausgabe)
    bool colored = false;                                       // standardmäßig keine farbige Ausgabe
    bool gif = false;                                           // standardmäßig wird nicht davon ausgegangen das der input ein GIF ist
    bool keep_frames = false;                                   // behalte temporäre Frames standardmäßig nicht
};

Config parse_args(int argc, char* argv[]);

void set_defaults(Config &cfg, const char* exe_path);

void validate_config(const Config &cfg);

std::string render_ascii(const Config &cfg);

void output_ascii(const std::string &ascii, const Config &cfg);

#endif //IMG_TO_ASCII_GENERAL_UTILS_H
