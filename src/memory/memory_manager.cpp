#include "memory/memory_manager.hpp"
#include "core/process.hpp"
#include <algorithm>
#include <iostream>
#include <random>
#include <limits>

namespace OSSimulator {

MemoryManager::MemoryManager(int total_frames, std::unique_ptr<ReplacementAlgorithm> algo)
    : total_frames(total_frames), algorithm(std::move(algo)) {
    frames.resize(total_frames);
    for (int i = 0; i < total_frames; ++i) {
        frames[i] = {i, -1, -1, false};
    }
}

void MemoryManager::register_process(std::shared_ptr<Process> process) {
    std::lock_guard<std::mutex> lock(mutex_);
    process_map[process->pid] = process;
}

void MemoryManager::unregister_process(int pid) {
    std::lock_guard<std::mutex> lock(mutex_);
    process_map.erase(pid);
    
    // Release memory internal
    for (auto& frame : frames) {
        if (frame.process_id == pid) {
            frame.process_id = -1;
            frame.page_id = -1;
            frame.occupied = false;
            if (algorithm) algorithm->on_frame_release(frame.frame_id);
        }
    }
}

bool MemoryManager::allocate_initial_memory(Process& process) {
    // Initialize page table based on memory_required
    // Assuming memory_required is number of pages for simplicity
    int num_pages = process.memory_required;
    process.page_table.resize(num_pages);
    for(int i=0; i<num_pages; ++i) {
        process.page_table[i] = Page(i);
        process.page_table[i].process_id = process.pid;
    }
    return true; 
}

bool MemoryManager::request_page(Process& process, int page_id, bool is_write, int current_time) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (page_id < 0 || static_cast<size_t>(page_id) >= process.page_table.size()) {
        return false; // Invalid page
    }

    Page& page = process.page_table[page_id];
    
    if (page.valid) {
        // Page hit
        page.referenced = true;
        if (is_write) page.modified = true;
        page.last_access_time = current_time;
        return true;
    }
    
    // Page fault
    return false;
}

void MemoryManager::handle_page_fault(Process& process, int page_id, int current_time) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    process.page_faults++;
    total_page_faults++;
    
    int frame_idx = find_free_frame();
    
    if (frame_idx == -1) {
        // Replacement needed
        process.replacements++;
        total_replacements++;
        frame_idx = algorithm->select_victim(frames, process_map, current_time);
        
        // Evict victim
        Frame& frame = frames[frame_idx];
        if (frame.process_id != -1) {
            auto it = process_map.find(frame.process_id);
            if (it != process_map.end()) {
                Process& victim_proc = *it->second;
                if (frame.page_id >= 0 && static_cast<size_t>(frame.page_id) < victim_proc.page_table.size()) {
                    Page& victim_page = victim_proc.page_table[frame.page_id];
                    victim_page.valid = false;
                    victim_page.frame_number = -1;
                    victim_proc.active_pages_count--;
                }
            }
        }
    }
    
    // Load new page
    Frame& frame = frames[frame_idx];
    frame.process_id = process.pid;
    frame.page_id = page_id;
    frame.occupied = true;
    
    Page& page = process.page_table[page_id];
    page.valid = true;
    page.frame_number = frame_idx;
    page.referenced = true; // Just loaded
    page.last_access_time = current_time;
    process.active_pages_count++;
    
    if (algorithm) algorithm->on_page_access(frame_idx);
}

void MemoryManager::release_process_memory(int pid) {
    unregister_process(pid);
}

int MemoryManager::find_free_frame() {
    for (int i = 0; i < total_frames; ++i) {
        if (!frames[i].occupied) return i;
    }
    return -1;
}

int MemoryManager::get_total_page_faults() const { return total_page_faults; }
int MemoryManager::get_total_replacements() const { return total_replacements; }

}
