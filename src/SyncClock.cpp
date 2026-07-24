//
// Created by lupo on 25.07.26.
//
#include "SyncClock.hpp"

#include "Verbose.hpp"

void SyncClock::start() {
    if (is_started) {
        throw std::runtime_error("SyncClock is already started!");
    }
    DEBUG("SyncClock started.");
    start_time = std::chrono::steady_clock::now();
    is_started = true;
}


double SyncClock::now_ms() const {
    if (!is_started) {
        throw std::runtime_error("SyncClock is not started! Call start() first.");
    }
    DEBUG("SyncClock: now_ms got called");
    const auto t = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(t - start_time.load()).count();
}


void SyncClock::wait_until(const double target_ms) const {
    if (!is_started) {
        throw std::runtime_error("SyncClock is not started! Call start() first.");
    }
    DEBUG("SyncClock: wait_until got called");
    using namespace std::chrono;

    const auto target_tp = start_time.load() + duration<double, std::milli>(target_ms);
    const auto now = steady_clock::now();

    // Adaptive Sleep: Erst grobe Sleep, dann Busy-Wait für die letzten Mikrosekunden
    if (target_tp > now + 1ms) {
        std::this_thread::sleep_until(target_tp - 1ms);// Sleep bis 1ms vor dem Ziel
    }

    // Busy-Wait für die letzten Mikrosekunden (präziser, aber CPU-lastig)
    while (steady_clock::now() < target_tp) {
        std::this_thread::sleep_for(100ns);// CPU an andere Threads abgeben
    }
}
