//
// Created by lupo on 25.07.26.
//

/**
 * @file SyncClock.cpp
 * @brief High-precision timing primitives used for frame/audio synchronization.
 *
 * Provides a steady-clock based time reference that supports waiting until
 * an absolute millisecond timestamp (relative to start()). The implementation
 * uses an adaptive sleep strategy: coarse sleep followed by a short busy-wait
 * to achieve sub-millisecond accuracy.
 */
#include <SyncClock.hpp>
#include <Verbose.hpp>

/**
 * @brief Start or reset the clock reference point.
 *
 * Records the current steady_clock time as the zero point for subsequent
 * now_ms() and wait_until() calls. Throws if the clock is already started.
 */
void SyncClock::start() {
    if (is_started) { throw std::runtime_error("SyncClock is already started!"); }
    DEBUG("SyncClock started.");
    start_time = std::chrono::steady_clock::now();
    is_started = true;
}


/**
 * @brief Return elapsed time in milliseconds since start().
 *
 * Uses steady_clock to measure elapsed time. Throws if start() was not called.
 */
double SyncClock::now_ms() const {
    if (!is_started) { throw std::runtime_error("SyncClock is not started! Call start() first."); }
    DEBUG("SyncClock: now_ms got called");
    const auto t = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(t - start_time.load()).count();
}


/**
 * @brief Sleep until an absolute target time (ms since start()) is reached.
 *
 * Implements an adaptive strategy: sleep until ~1ms before the target and
 * then busy-wait (with short sleeps) to reach the precise moment. Throws if
 * start() was not called.
 *
 * @param target_ms Absolute time in milliseconds since start() to wait until.
 */
void SyncClock::wait_until(const double target_ms) const {
    if (!is_started) { throw std::runtime_error("SyncClock is not started! Call start() first."); }
    DEBUG("SyncClock: wait_until got called");
    using namespace std::chrono;

    const auto target_tp = start_time.load() + duration<double, std::milli>(target_ms);

    // Adaptive Sleep: Erst grobe Sleep, dann Busy-Wait für die letzten Mikrosekunden
    if (target_tp > steady_clock::now() + 1ms) {
        std::this_thread::sleep_until(target_tp - 1ms);// Sleep bis 1ms vor dem Ziel
    }

    // Busy-Wait für die letzten Mikrosekunden (präziser, aber CPU-lastig)
    while (steady_clock::now() < target_tp) {
        std::this_thread::sleep_for(100ns);// CPU an andere Threads abgeben
    }
}
