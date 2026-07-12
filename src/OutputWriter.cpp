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


void OutputWriter::push(std::string data) {
    {
        std::lock_guard lock(mutex);

        queue.push(std::move(data));
    }

    condition.notify_one();
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

            std::string data = std::move(queue.front());
            queue.pop();

            lock.unlock();

            write_stdout(data);

            lock.lock();
        }
    }
}