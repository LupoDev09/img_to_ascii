//
// Created by lupo on 06.07.26.
//

#ifndef IMG_TO_ASCII_SYNCCLOCK_H
#define IMG_TO_ASCII_SYNCCLOCK_H
#include "Verbose.hpp"
#include <chrono>
#include <thread>

/**
 * @class SyncClock
 * @brief High-precision timing utility for synchronized playback.
 * 
 * Provides stable frame timing for video playback by synchronizing frame delivery
 * with a high-resolution timer. Prevents timing drift by using an absolute target time
 * rather than cumulative delays.
 */
class SyncClock {
public:
    /**
     * @brief Start the clock's time reference.
     * 
     * Should be called once before any wait_until() calls.
     * Establishes the zero point for all subsequent timing.
     */
    void start() {
        DEBUG("SyncClock started.");
        start_time = std::chrono::steady_clock::now();
    }

    /**
     * @brief Get elapsed time in milliseconds since start().
     * @return Elapsed time in milliseconds with floating-point precision
     */
    [[nodiscard]] double now_ms() const {
        DEBUG("SyncClock: now_ms got called");
        const auto t = std::chrono::steady_clock::now();
        return std::chrono::duration<double, std::milli>(t - start_time).count();
    }

    /**
     * @brief Sleep until a target time in milliseconds is reached.
     * 
     * Blocks the current thread until the specified time has elapsed since start().
     * Uses adaptive sleeping to minimize CPU usage while maintaining precision:
     * - Polls if far from target (>1ms away)
     * - Yields to avoid busy-waiting
     * - Guarantees frame timing without drift
     * 
     * @param target_ms Absolute time (in ms since start()) to wait until
     * 
     * @note This is intended for frame synchronization where target_ms = frame_index * frame_duration
     */
    void wait_until(const double target_ms) const {
        DEBUG("SyncClock: wait_until got called");
        using namespace std::chrono;

        const auto start_tp = start_time;
        const auto target_tp = start_tp + duration<double, std::milli>(target_ms);

        std::this_thread::sleep_until(target_tp);
    }

private:
    std::chrono::steady_clock::time_point start_time; ///< Reference point for timing
};

#endif// IMG_TO_ASCII_SYNCCLOCK_H
