//
// Created by lupo on 12.07.26.
//

/**
 * @file OutputWriter.cpp
 * @brief Asynchronous, timed writing of ANSI-formatted frame data to stdout.
 *
 * The OutputWriter exposes a thread-safe queue for writing strings to stdout.
 * Items can optionally carry an absolute target time (ms since a SyncClock start)
 * and the worker will sleep until that time before emitting the data.
 */

#include <OutputWriter.hpp>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif


/**
 * @brief Low-level platform specific write to stdout.
 *
 * Uses WriteFile on Windows and POSIX write() on Unix-like systems to avoid
 * iostream buffering overhead for high-frequency writes.
 */
static void write_stdout(const std::string& data) {
#ifdef _WIN32

    DWORD written = 0;
    HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);

    WriteFile(stdout_handle, data.data(), static_cast<DWORD>(data.size()), &written, nullptr);

#else

    write(STDOUT_FILENO, data.data(), data.size());

#endif
}


OutputWriter::OutputWriter() = default;


OutputWriter::~OutputWriter() { stop(); }


void OutputWriter::set_clock(SyncClock& clock) { this->clock_ = &clock; }


bool OutputWriter::push(std::string data, std::optional<double> target_ms) {
    {
        std::lock_guard lock(mutex);

        if (MAX_QUEUE_SIZE > 0 && queue.size() >= MAX_QUEUE_SIZE) { return false; }
        queue.push(QueuedWrite{.data = std::move(data), .target_ms = target_ms});
    }

    condition.notify_one();
    return true;
}

void OutputWriter::start() { thread = std::thread(&OutputWriter::worker, this); }

void OutputWriter::stop() {
    if (!running.exchange(false)) return;

    condition.notify_one();

    if (thread.joinable()) thread.join();
}


/**
 * @brief Worker loop that consumes the write queue and emits data to stdout.
 *
 * The worker optionally synchronizes each output item to an absolute target
 * time using the associated SyncClock. The function runs until running == false
 * and the queue is drained.
 */
void OutputWriter::worker() {
    while (running || !queue.empty()) {

        std::unique_lock lock(mutex);

        condition.wait(lock, [&] { return !queue.empty() || !running; });

        while (!queue.empty()) {
            auto [data, target_ms] = queue.front();
            queue.pop();

            if (clock_ != nullptr && target_ms.has_value()) {
                lock.unlock();
                clock_->wait_until(*target_ms);
                lock.lock();
            }

            if (!running && queue.empty()) { break; }

            write_stdout(data);
        }
    }
}