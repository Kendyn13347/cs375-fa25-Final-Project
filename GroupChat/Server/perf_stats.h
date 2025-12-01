// Server/perf_stats.h
#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

// ---- Initialization ----
void stats_init();                  // call once at server startup
void stats_init_threads(std::size_t numThreads);  // from ThreadPool ctor
void stats_init_vm(std::size_t capacity);         // e.g. 16 or 32 "pages"

// ---- Message rate ----
void stats_record_message();        // call whenever a chat MESSAGE is processed
double stats_get_message_rate();

// ---- Cache stats ----
void stats_record_cache_hit(std::uint16_t groupId);
void stats_record_cache_miss(std::uint16_t groupId);

// ---- Thread / queue stats ----
void stats_record_task_completed(std::size_t threadIndex);
void stats_record_queue_size(std::size_t queueSize);

// ---- Virtual memory (paging simulation) ----
void stats_vm_access(int pageId);   // simulate referencing a "page"

// ---- Reporting ----
void stats_dump_to_stdout();
void stats_dump_to_file(const std::string& path);
