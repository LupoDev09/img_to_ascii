//
// Created by lupo on 06.07.26.
//

#ifndef IMG_TO_ASCII_SYNCCLOCK_H
#define IMG_TO_ASCII_SYNCCLOCK_H
#include <chrono>
#include <thread>

/**
 * @class SyncClock
 * @brief High-precision timing utility for synchronized playback.
 *
 * @note This class is thread-safe if used correctly (each thread should have its own instance).
 * @throws std::runtime_error if methods are called before start().
 *
 * @example
 * SyncClock clock;
 * clock.start();
 * clock.wait_until(1000.0); // Warte 1 Sekunde
 */
class SyncClock {
public:
    SyncClock() : start_time(std::chrono::steady_clock::now()) {} // Standardkonstruktor setzt start_time

    /**
     * @brief Start the clock's time reference.
     * @throws std::runtime_error if the clock is already started.
     */
    void start();

    /**
     * @brief Get elapsed time in milliseconds since start().
     * @return Elapsed time in milliseconds with floating-point precision.
     * @throws std::runtime_error if the clock is not started.
     */
    [[nodiscard]] double now_ms() const;

    /**
     * @brief Sleep until a target time in milliseconds is reached.
     * @param target_ms Absolute time (in ms since start()) to wait until.
     * @throws std::runtime_error if the clock is not started.
     */
    void wait_until(double target_ms) const;

private:
    std::atomic<std::chrono::steady_clock::time_point> start_time; ///< Reference point for timing
    bool is_started = false; ///< Flag, ob die Uhr gestartet wurde
};

#endif// IMG_TO_ASCII_SYNCCLOCK_H
