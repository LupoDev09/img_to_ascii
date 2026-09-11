//
// Created by lupo on 12.07.26.
//

#include <OutputWriter.hpp>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif


static void write_stdout(const std::string &data) {
#ifdef _WIN32

    DWORD written = 0;
    HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);

    WriteFile(
            stdout_handle,
            data.data(),
            static_cast<DWORD>(data.size()),
            &written,
            nullptr);

#else

    write(
            STDOUT_FILENO,
            data.data(),
            data.size());

#endif
}


OutputWriter::OutputWriter() = default;


OutputWriter::~OutputWriter() {
    stop();
}


void OutputWriter::set_clock(SyncClock &clock) {
    this->clock_ = &clock;
}


bool OutputWriter::push(std::string data, std::optional<double> target_ms) {
    {
        std::lock_guard lock(mutex);

        if (MAX_QUEUE_SIZE > 0 && queue.size() >= MAX_QUEUE_SIZE) {
            return false;
        }
        queue.push(QueuedWrite{.data = std::move(data), .target_ms = target_ms});
    }

    condition.notify_one();
    return true;
}

void OutputWriter::start() {
    thread = std::thread(&OutputWriter::worker, this);
}

void OutputWriter::stop() {
    if (!running.exchange(false))
        return;

    condition.notify_one();

    if (thread.joinable())
        thread.join();
}


void OutputWriter::worker() {
    while (running || !queue.empty()) {

        std::unique_lock lock(mutex);

        condition.wait(lock, [&] {
            return !queue.empty() || !running;
        });


        while (!queue.empty()) {
            auto [data, target_ms] = queue.front();
            queue.pop();

            if (clock_ != nullptr && target_ms.has_value()) {
                lock.unlock();
                clock_->wait_until(*target_ms);
                lock.lock();
            }

            if (!running && queue.empty()) {
                break;
            }

            write_stdout(data);
        }
    }
}