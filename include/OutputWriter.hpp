//
// Created by lupo on 12.07.26.
//

#ifndef IMG_TO_ASCII_OUTPUTWRITER_HPP
#define IMG_TO_ASCII_OUTPUTWRITER_HPP

#include <SyncClock.hpp>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <thread>

class OutputWriter {
public:
    OutputWriter();
    ~OutputWriter();

    // Keine Kopien oder Zuweisungen
    OutputWriter(const OutputWriter &) = delete;
    OutputWriter &operator=(const OutputWriter &) = delete;
    OutputWriter(OutputWriter &&) = delete;
    OutputWriter &operator=(OutputWriter &&) = delete;

    void set_clock(SyncClock &clock);

    /**
     * @brief Push data to the output queue for asynchronous writing.
     * @param data the data to output
     * @param target_ms optional absolute playback time in ms since start
     */
    [[nodiscard]] bool push(std::string data, std::optional<double> target_ms = std::nullopt);

    /**
     * @brief Starts the output writer thread.
     * This thread will continuously write data from the queue to stdout.
     */
    void start();

    /**
     * @brief Stops the output writer thread.
     */
    void stop();

private:
    /**
     * @brief Worker function that runs in a separate thread.
     * It continuously checks the queue for new data and writes it to stdout.
     */
    void worker();

    struct QueuedWrite {
        std::string data;
        std::optional<double> target_ms;
    };

    // The queue that holds the data to be written to stdout
    std::queue<QueuedWrite> queue;

    std::mutex mutex; // A lock for the queue
    std::condition_variable condition; // IDK what this does, I guess it's something that the worker uses to wait

    std::thread thread;               // The worker thread
    std::atomic<bool> running = true; // Flag to control the running state of the worker thread
    SyncClock *clock_ = nullptr;

    const uint8_t MAX_QUEUE_SIZE = 20; // Maximum number of items in the queue
};



#endif// IMG_TO_ASCII_OUTPUTWRITER_HPP
