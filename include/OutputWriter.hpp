//
// Created by lupo on 12.07.26.
//

#ifndef IMG_TO_ASCII_OUTPUTWRITER_HPP
#define IMG_TO_ASCII_OUTPUTWRITER_HPP

#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <atomic>

class OutputWriter {
public:
    OutputWriter();
    ~OutputWriter();

    // Keine Kopien oder Zuweisungen
    OutputWriter(const OutputWriter &) = delete;
    OutputWriter &operator=(const OutputWriter &) = delete;
    OutputWriter(OutputWriter &&) = delete;
    OutputWriter &operator=(OutputWriter &&) = delete;


    /**
     * @brief Push data to the output queue for asynchronous writing.
     * @param data the data to output
     */
    void push(std::string data);

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

    // The queue that holds the data to be written to stdout
    std::queue<std::string> queue;

    std::mutex mutex; // A lock for the queue
    std::condition_variable condition; // IDK what this does, I guess it's something that the worker uses to wait

    std::thread thread;               // The worker thread
    std::atomic<bool> running = true; // Flag to control the running state of the worker thread

};



#endif// IMG_TO_ASCII_OUTPUTWRITER_HPP
