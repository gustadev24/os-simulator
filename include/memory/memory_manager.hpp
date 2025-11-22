#ifndef MEMORY_MANAGER_HPP
#define MEMORY_MANAGER_HPP

#include "memory/page.hpp"
#include "memory/replacement_algorithm.hpp"
#include <vector>
#include <mutex>
#include <unordered_map>
#include <list>
#include <memory>
#include <algorithm>

namespace OSSimulator {

// Forward declaration to avoid circular dependency
struct Process;

class MemoryManager {
public:
    MemoryManager(int total_frames, std::unique_ptr<ReplacementAlgorithm> algo);

    // Register a process to be managed (needed for victim selection lookups)
    void register_process(std::shared_ptr<Process> process);
    void unregister_process(int pid);

    // Try to allocate initial pages. Returns true if successful.
    bool allocate_initial_memory(Process& process);

    // Request access to a page. 
    // Returns true if page is in memory.
    // Returns false if page fault occurred (caller should block process).
    bool request_page(Process& process, int page_id, bool is_write, int current_time);
    
    // Handle the page fault (load page, potentially replacing another).
    // Should be called when the process is ready to handle the fault (or immediately if sync).
    void handle_page_fault(Process& process, int page_id, int current_time);

    // Release all frames held by process
    void release_process_memory(int pid);

    // Metrics
    int get_total_page_faults() const;
    int get_total_replacements() const;

private:
    int total_frames;
    std::unique_ptr<ReplacementAlgorithm> algorithm;
    
    std::vector<Frame> frames;
    std::unordered_map<int, std::shared_ptr<Process>> process_map;
    std::mutex mutex_;
    
    // Metrics
    int total_page_faults = 0;
    int total_replacements = 0;

    int find_free_frame();
};

}

#endif
