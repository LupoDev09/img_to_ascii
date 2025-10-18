//#define STB_IMAGE_WRITE_IMPLEMENTATION  // Implementation wird in stb_impl.cpp bereitgestellt
#include <filesystem>
#include <iostream>
#include <string>
#include "../include/img_utils.h"
#include "../include/general_utils.h"

using namespace std;
namespace fs = std::filesystem;


int main(int argc, char *argv[]) {
    try {
        enable_vt_mode();
        Config cfg = parse_args(argc, argv);
        set_defaults(cfg, argv[0]);
        validate_config(cfg);

        std::string ascii = render_ascii(cfg);
        output_ascii(ascii, cfg);

    } catch (const std::exception &e) {
        std::cerr << e.what() << "\n";
        print_help();
        std::cout << "\033[0m" << std::endl;
        return 1;
    }
    std::cout << "\033[0m" << std::endl;
    return 0;
}

// TODO: Extract frame delays / disposal info and save as JSON alongside frames. This will allow accurate playback timing later. (Nice to have.)
// TODO: Try to use a C++ image library to extract GIF frames directly instead of relying on ImageMagick. (Harder :3)
