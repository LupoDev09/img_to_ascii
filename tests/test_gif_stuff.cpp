//
// Created by lupo on 21.10.25.
//

#include <catch2/catch.hpp>
#include <cstdlib>
#include <string>
#include <memory>
#include <iostream>
#include "test_utils.h"

// Zusätzliche Testfälle für FPS-Optionen
TEST_CASE("FPS als nicht-numerischer Wert wird korrekt behandelt", "[cli]") {
    std::string output = run_cmd("./img_to_ascii --gif --fps abc");
    REQUIRE(output.find("Argument") != std::string::npos);
    REQUIRE(output.find("failed to parse") != std::string::npos);
}

TEST_CASE("GIF-Option ohne Bilddatei wird korrekt behandelt", "[cli]") {
    std::string output = run_cmd("./img_to_ascii --gif");
    REQUIRE(output.find("Kein Bildpfad angegeben") != std::string::npos);
}
