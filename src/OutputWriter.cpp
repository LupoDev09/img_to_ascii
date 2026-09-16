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
#include <Verbose.hpp>
#include <fstream>

/**
 * @brief Writes to the output stream
 */
void OutputWriter::write_output(const std::string& data) {
    if (output_stream) {
        output_stream.write(data.data(), static_cast<std::streamsize>(data.size()));
        if (output_mode_ != DataStructures::Output::FILE) {
            output_stream.flush();
        }
    }
}


OutputWriter::OutputWriter(const DataStructures::Output output_mode, const std::string& output_file)
    : output_stream(nullptr), output_mode_(output_mode) {// Buffer wird unten je nach Modus gesetzt
    switch (output_mode) {
        case DataStructures::Output::FILE:
            file_stream.open(output_file, std::ios::out);
            if (!file_stream.is_open()) {
                throw std::runtime_error("Failed to open output file: " + output_file);
            }
            output_stream.rdbuf(file_stream.rdbuf());
            break;
        case DataStructures::Output::STDOUT:
            output_stream.rdbuf(std::cout.rdbuf());
            break;
        case DataStructures::Output::NO_OUTPUT:
            output_stream.rdbuf(nullptr);// Schreibvorgänge landen im Leeren
            break;
    }
};


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

void OutputWriter::start() {
    thread = std::thread(&OutputWriter::worker, this);
    SET_THREAD_NAME(thread, "OutputWriter");
}

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

        if (output_mode_ == DataStructures::Output::STDOUT) {
            while (!queue.empty()) {
                auto [data, target_ms] = queue.front();
                queue.pop();

                if (clock_ != nullptr && target_ms.has_value()) {
                    lock.unlock();
                    clock_->wait_until(*target_ms);
                    lock.lock();
                }

                write_output(data);
            }
        } else {
            // Kein Live-Betrachter, der exaktes Timing braucht -> alles, was
            // gerade wartet, zu einem einzigen Write bündeln statt pro Frame
            // einen eigenen Syscall auszulösen.
            std::string batch;
            while (!queue.empty()) {
                batch += queue.front().data;
                queue.pop();
            }
            lock.unlock();
            if (!batch.empty()) { write_output(batch); }
        }
    }
}