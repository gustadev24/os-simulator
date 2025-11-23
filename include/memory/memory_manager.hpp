#ifndef MEMORY_MANAGER_HPP
#define MEMORY_MANAGER_HPP

#include "memory/page.hpp"
#include "memory/replacement_algorithm.hpp"
#include <algorithm>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <vector>

namespace OSSimulator {

// Forward declaration to avoid circular dependency
struct Process;

class MemoryManager {
public:
    using ProcessReadyCallback = std::function<void(std::shared_ptr<Process>)>;

    MemoryManager(int total_frames, std::unique_ptr<ReplacementAlgorithm> algo,
                  int page_fault_latency = 1);

    // Register a process to be managed (needed for victim selection lookups)
    void register_process(std::shared_ptr<Process> process);
    void unregister_process(int pid);

    void set_ready_callback(ProcessReadyCallback callback);

    // Try to allocate initial pages. Returns true if successful.
    bool allocate_initial_memory(Process& process);

    // Ensure that every page required by the process is resident before CPU use.
    // Returns true if the process can execute immediately, false if it was queued for loading.
    bool prepare_process_for_cpu(std::shared_ptr<Process> process, int current_time);

    // Advance the asynchronous page-fault queue by a duration.
    void advance_fault_queue(int duration, int start_time);

    // Mark all of a process' pages as evictable (used when it leaves the CPU).
    void mark_process_inactive(const Process& process);

    // Release all frames held by process
    void release_process_memory(int pid);

    // Metrics
    int get_total_page_faults() const;
    int get_total_replacements() const;

private:
    int total_frames;
    std::unique_ptr<ReplacementAlgorithm> algorithm;
    int page_fault_latency;
    
    std::vector<Frame> frames;
    std::unordered_map<int, std::shared_ptr<Process>> process_map;
    std::mutex mutex_;

    struct PageLoadTask {
        std::shared_ptr<Process> process;
        int page_id;
        int remaining_time;
        int frame_id;
        int enqueue_time;
    };

    std::deque<PageLoadTask> fault_queue;
    std::optional<PageLoadTask> active_task;
    std::unordered_map<int, std::unordered_set<int>> pending_pages_by_process;
    std::unordered_set<int> processes_waiting_on_memory;
    ProcessReadyCallback ready_callback;
    int memory_time = 0;
    
    // Metrics
    int total_page_faults = 0;
    int total_replacements = 0;

    int find_free_frame();
    bool are_all_pages_resident(const Process& process) const;
    void enqueue_missing_pages(std::shared_ptr<Process> process,
                               const std::vector<int>& missing_pages,
                               int current_time);
    void start_next_task_if_possible(int current_time);
    bool reserve_frame_for_task(PageLoadTask& task);
    std::shared_ptr<Process> complete_active_task(int completion_time);
    void evict_frame(int frame_idx);
    void set_process_pages_referenced(const Process& process, bool referenced);
};

}

#endif
