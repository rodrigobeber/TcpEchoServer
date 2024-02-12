#include "ThreadPool.h"

// Constructor, create the threads
ThreadPool::ThreadPool(size_t threads) : processStop(false) {
    for (size_t i = 0; i < threads; ++i) {
        workers.emplace_back(&ThreadPool::processTasks, this);
    }
}

// Destructor, join the threads (wait them to finish)
ThreadPool::~ThreadPool() {
    stop();
    for (std::thread &worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

// Get pool size
size_t ThreadPool::getThreadCount() const {
    return workers.size();
}

// Add a task to the queue
void ThreadPool::execute(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        tasks.emplace(std::move(task));
    }
    // Notify that a new task has been placed
    taskCondition.notify_one();
}

// Stop the thread pool (gracefuly)
void ThreadPool::stop() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        processStop = true;
    }
    // Notify all thread to stop
    taskCondition.notify_all();
}

// Each thread will process this loop trying to assing a task to itself
void ThreadPool::processTasks() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            // Wait for having a task or for the stop signal (if wait, queue_mutex is released)
            this->taskCondition.wait(lock, [this] { return this->processStop || !this->tasks.empty(); });
            // Graceful stop (will process all tasks first)
            if (this->processStop && this->tasks.empty()) {
                return; // Finish the loop
            }
            // Extract the task from the queue
            task = std::move(this->tasks.front());
            this->tasks.pop();
        }
        // Execute the task
        task();
    }
}
