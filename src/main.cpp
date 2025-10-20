//#define STB_IMAGE_WRITE_IMPLEMENTATION  // Implementation wird in stb_impl.cpp bereitgestellt
#include <iostream>
#include <string>
#include "../include/general_utils.h"

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
        enable_vt_mode();                       // ANSI-Escape-Sequenzen aktivieren (Windows)
        Config cfg = parse_args(argc, argv);    // Kommandozeilenargumente parsen
        set_defaults(cfg, argv[0]);          // Standardwerte setzen
        validate_config(cfg);                   // Konfiguration validieren

        const string ascii = render_ascii(cfg); // ASCII-Art generieren
        output_ascii(ascii, cfg);               // ASCII-Art ausgeben

    } catch (const exception &e) {
        cerr << e.what() << "\n";               // Fehler ausgeben
        cout << "\033[0m" << endl;              // ANSI-Reset
        return 1;
    }
    cout << "\033[0m" << endl;                  // ANSI-Reset
    return 0;
}
