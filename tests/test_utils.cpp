//
// Created by lupo on 21.10.25.
//
#include <catch2/catch_all.hpp>
#include <string>
#include <array>
#include <memory>
#include <iostream>
#include "test_utils.h"

std::string run_cmd(const std::string& cmd) {
    std::array<char, 128> buffer{};
    std::string result;
    // stderr (2) -> stdout (1) umleiten
    std::unique_ptr<FILE, decltype(+pclose)> pipe(popen((cmd + " 2>&1").c_str(), "r"), +pclose);
    if (!pipe) throw std::runtime_error("popen() failed!");
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
        result += buffer.data();
    return result;
}