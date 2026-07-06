//
// Created by lupo on 06.07.26.
//

#ifndef IMG_TO_ASCII_SYNCCLOCK_H
#define IMG_TO_ASCII_SYNCCLOCK_H

#pragma once
#include <chrono>
#include <thread>

class SyncClock {
public:
    void start() {
        start_time = std::chrono::steady_clock::now();
    }

    double now_ms() const {
        const auto t = std::chrono::steady_clock::now();
        return std::chrono::duration<double, std::milli>(t - start_time).count();
    }

    void wait_until(const double target_ms) const {
        // sorgt für stabile Frame-Timing ohne Drift
        while (true) {
            const double current = now_ms();
            if (current >= target_ms) break;

            const double sleep_ms = target_ms - current;
            std::this_thread::sleep_for(
                std::chrono::milliseconds(static_cast<int>(sleep_ms))
            );
        }
    }

private:
    std::chrono::steady_clock::time_point start_time;
};

#endif// IMG_TO_ASCII_SYNCCLOCK_H
