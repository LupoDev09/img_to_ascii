// #define STB_IMAGE_WRITE_IMPLEMENTATION  // Implementation wird in stb_impl.cpp bereitgestellt
#include "../include/general_utils.h"
#include <iostream>
#include <string>

using namespace std;

/**
 * @brief Hauptfunktion der Anwendung
 *
 * @param argc Anzahl der Kommandozeilenargumente
 * @param argv Array der Kommandozeilenargumente
 *
 * @return Rückgabewert (0 bei Erfolg, 1 bei Fehler)
 */
int main(const int argc, char *argv[]) {
    try {
#if defined(_WIN32)
        enable_vt_mode();// ANSI-Escape-Sequenzen aktivieren (Windows)
#endif
        Config cfg = parse_args(argc, argv);// Kommandozeilenargumente parsen
        if (cfg.load_config != "") {

            if (std::filesystem::exists(cfg.load_config)) {
                cout << "Konfigurationsdatei geladen: " << cfg.load_config << "\n"
                     << "Aktuelle Konfiguration:\n"
                     << "  Bildpfad: " << cfg.image_path << "\n"
                     << "  Breite: " << cfg.width << "\n"
                     << "  ASCII-Zeichen: " << cfg.ascii_chars << "\n"
                     << "  Farbig: " << (cfg.colored ? "Ja" : "Nein") << "\n"
                     << "  GIF-Modus: " << (cfg.gif ? "Ja" : "Nein") << "\n"
                     << "  Frame-Rate: " << cfg.fps << "\n"
                     << "  Loop: " << cfg.loop
                     << endl;
            } else {
                throw runtime_error("Config nicht gefunden: " + cfg.load_config.string());
            }
        }

        set_defaults(cfg);   // Standardwerte setzen
        validate_config(cfg);// Konfiguration validieren

        const string ascii = render_ascii(cfg);// ASCII-Art generieren

        if (cfg.no_output == true) return 0;// Wenn no_output auf true nich ausgeben sondern abbrechen

        output_ascii(ascii, cfg);// ASCII-Art ausgeben

    } catch (const exception &e) {
        cerr << e.what() << "\n"; // Fehler ausgeben
        cout << "\033[0m" << endl;// ANSI-Reset
        return 1;
    }
    cout << "\033[0m" << endl;// ANSI-Reset
    return 0;
}
