// Server/perf_stats.cpp
#include "perf_stats.h"

#include <atomic>
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <vector>

using Clock = std::chrono::steady_clock;

// ---- message rate ----
static std::atomic<std::uint64_t> g_messageCount{0};
static Clock::time_point g_startTime;

// ---- cache stats ----
struct CacheStats {
    std::uint64_t hits  = 0;
    std::uint64_t misses = 0;
};
static std::map<std::uint16_t, CacheStats> g_cacheStats;
static std::mutex g_cacheMutex;

// ---- thread / queue stats ----
static std::vector<std::uint64_t> g_tasksPerThread;
static std::mutex g_tasksMutex;
static std::atomic<std::uint64_t> g_maxQueueSize{0};


// ---- virtual memory / paging simulation ----
struct Page {
    int pageId = -1;
    bool valid = false;
    std::uint64_t lastUsed = 0;
};

class VirtualMemory {
public:
    VirtualMemory() : capacity(0), timeCounter(0), pageFaults(0) {}

    void init(std::size_t cap) {
        capacity = cap;
        frames.assign(capacity, Page{});
        timeCounter = 0;
        pageFaults = 0;
    }

    void access(int pageId) {
        if (capacity == 0) return;
        timeCounter++;

        // hit?
        for (auto& f : frames) {
            if (f.valid && f.pageId == pageId) {
                f.lastUsed = timeCounter;
                return;
            }
        }

        // miss -> page fault
        pageFaults++;

        // choose LRU or empty frame
        Page* victim = &frames[0];
        for (auto& f : frames) {
            if (!f.valid || f.lastUsed < victim->lastUsed) {
                victim = &f;
            }
        }
        victim->valid = true;
        victim->pageId = pageId;
        victim->lastUsed = timeCounter;
    }

    std::uint64_t getPageFaults() const { return pageFaults; }

private:
    std::size_t capacity;
    std::vector<Page> frames;
    std::uint64_t timeCounter;
    std::uint64_t pageFaults;
};

static VirtualMemory g_vm;

// ---- API implementation ----

void stats_init() {
    g_startTime = Clock::now();
    g_messageCount.store(0);
    g_maxQueueSize.store(0);
}


void stats_init_threads(std::size_t numThreads) {
    std::lock_guard<std::mutex> lock(g_tasksMutex);
    g_tasksPerThread.clear();
    g_tasksPerThread.resize(numThreads, 0);   // now OK: plain uint64_t
}




void stats_init_vm(std::size_t capacity) {
    g_vm.init(capacity);
}

void stats_record_message() {
    g_messageCount++;
}

double stats_get_message_rate() {
    auto now = Clock::now();
    double seconds = std::chrono::duration<double>(now - g_startTime).count();
    if (seconds <= 0.0) return 0.0;
    return static_cast<double>(g_messageCount.load()) / seconds;
}

void stats_record_cache_hit(std::uint16_t groupId) {
    std::lock_guard<std::mutex> lock(g_cacheMutex);
    g_cacheStats[groupId].hits++;
}

void stats_record_cache_miss(std::uint16_t groupId) {
    std::lock_guard<std::mutex> lock(g_cacheMutex);
    g_cacheStats[groupId].misses++;
}

void stats_record_task_completed(std::size_t threadIndex) {
    std::lock_guard<std::mutex> lock(g_tasksMutex);
    if (threadIndex < g_tasksPerThread.size()) {
        g_tasksPerThread[threadIndex]++;
    }
}


void stats_record_queue_size(std::size_t queueSize) {
    std::uint64_t cur = g_maxQueueSize.load();
    while (queueSize > cur &&
           !g_maxQueueSize.compare_exchange_weak(cur, queueSize)) {
        // CAS loop
    }
}

void stats_vm_access(int pageId) {
    g_vm.access(pageId);
}

static void dumpToStream(std::ostream& os) {
    os << "=== Performance Stats ===\n";

    os << "Total messages: " << g_messageCount.load() << "\n";
    os << "Message rate: " << stats_get_message_rate() << " msg/sec\n\n";

    os << "--- Cache Stats (per group) ---\n";
    {
        std::lock_guard<std::mutex> lock(g_cacheMutex);
        for (auto& kv : g_cacheStats) {
            std::uint16_t gid = kv.first;
            auto& s = kv.second;
            os << "Group " << gid << "  hits=" << s.hits
               << "  misses=" << s.misses << "\n";
        }
    }
    os << "\n";

    os << "--- Thread usage ---\n";
{
    std::lock_guard<std::mutex> lock(g_tasksMutex);
    for (std::size_t i = 0; i < g_tasksPerThread.size(); ++i) {
        os << "Thread " << i << " completed "
           << g_tasksPerThread[i] << " tasks\n";
    }
}

    os << "Max queue size: " << g_maxQueueSize.load() << "\n\n";

    os << "--- Virtual Memory (simulated) ---\n";
    os << "Page faults: " << g_vm.getPageFaults() << "\n";
}

void stats_dump_to_stdout() {
    dumpToStream(std::cout);
}

void stats_dump_to_file(const std::string& path) {
    std::ofstream ofs(path, std::ios::out | std::ios::app);
    if (!ofs) return;
    dumpToStream(ofs);
    ofs << "\n";
}
