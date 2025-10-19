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
        enable_vt_mode();
        Config cfg = parse_args(argc, argv);
        set_defaults(cfg, argv[0]);
        validate_config(cfg);

        const string ascii = render_ascii(cfg);
        output_ascii(ascii, cfg);

    } catch (const exception &e) {
        cerr << e.what() << "\n";
        cout << "\033[0m" << endl;
        return 1;
    }
    cout << "\033[0m" << endl;
    return 0;
}
