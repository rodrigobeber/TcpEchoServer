#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

class ThreadPool {
private:
    // Threads
    std::vector<std::thread> workers;

    // Tasks queue
    std::queue<std::function<void()>> tasks;

    // Concurrency control
    std::mutex queue_mutex;
    std::condition_variable taskCondition;

    // Internal signal to graceful stop the tasks
    bool processStop;

    // Executed by each thread, loop trying to assign and process tasks until the stop signal
    void processTasks();
public:
    ThreadPool(size_t threads);
    ~ThreadPool();
    size_t getThreadCount() const;
    void execute(std::function<void()> task);
    void stop();
};