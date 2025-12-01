#include "thread_pool.h"
#include "perf_stats.h"

ThreadPool::ThreadPool(std::size_t threads) : stop(false) {
    stats_init_threads(threads);   // tell stats module how many threads we have

    for (std::size_t i = 0; i < threads; ++i) {
        workers.emplace_back([this, i] {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(queue_mutex);
                    condition.wait(lock, [this] {
                        return stop || !tasks.empty();
                    });
                    if (stop && tasks.empty()) return;
                    task = std::move(tasks.front());
                    tasks.pop();
                }
                task();
                stats_record_task_completed(i);
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for (auto& worker : workers) {
        worker.join();
    }
}

void ThreadPool::enqueue(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        tasks.push(std::move(task));
        stats_record_queue_size(tasks.size());
    }
    condition.notify_one();
}


// // server/thread_pool.cpp
// #include "thread_pool.h"

// ThreadPool::ThreadPool(std::size_t threads) : stop(false) {
//     for (std::size_t i = 0; i < threads; ++i) {
//         workers.emplace_back([this] {
//             while (true) {
//                 std::function<void()> task;
//                 {
//                     std::unique_lock<std::mutex> lock(queue_mutex);
//                     condition.wait(lock, [this] {
//                         return stop || !tasks.empty();
//                     });
//                     if (stop && tasks.empty())
//                         return;
//                     task = std::move(tasks.front());
//                     tasks.pop();
//                 }
//                 task();
//             }
//         });
//     }
// }

// ThreadPool::~ThreadPool() {
//     {
//         std::unique_lock<std::mutex> lock(queue_mutex);
//         stop = true;
//     }
//     condition.notify_all();
//     for (auto &worker : workers) {
//         worker.join();
//     }
// }

// void ThreadPool::enqueue(std::function<void()> task) {
//     {
//         std::unique_lock<std::mutex> lock(queue_mutex);
//         tasks.push(std::move(task));
//     }
//     condition.notify_one();
// }
