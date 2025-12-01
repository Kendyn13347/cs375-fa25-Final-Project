#pragma once

#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <vector>

class ThreadPool {
public:
    explicit ThreadPool(std::size_t threads);
    ~ThreadPool();

    void enqueue(std::function<void()> task);

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};


// // server/thread_pool.h
// #pragma once

// #include <thread>
// #include <queue>
// #include <mutex>
// #include <condition_variable>
// #include <functional>
// #include <vector>

// class ThreadPool {
// public:
//     explicit ThreadPool(std::size_t threads);
//     ~ThreadPool();

//     void enqueue(std::function<void()> task);

// private:
//     std::vector<std::thread> workers;                 // matches .cpp
//     std::queue<std::function<void()>> tasks;          // matches .cpp
//     std::mutex queue_mutex;                           // matches .cpp
//     std::condition_variable condition;                // matches .cpp
//     bool stop;                                        // matches .cpp
// };
