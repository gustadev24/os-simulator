#include "memory/memory_manager.hpp"
#include "core/process.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>

namespace OSSimulator {

MemoryManager::MemoryManager(int total_frames,
                             std::unique_ptr<ReplacementAlgorithm> algo,
                             int page_fault_latency)
    : total_frames(total_frames), algorithm(std::move(algo)),
      page_fault_latency(std::max(1, page_fault_latency)) {
  frames.resize(total_frames);
  for (int i = 0; i < total_frames; ++i) {
    frames[i] = {i, -1, -1, false};
  }
}

void MemoryManager::register_process(std::shared_ptr<Process> process) {
  if (!process)
    return;
  std::lock_guard<std::mutex> lock(mutex_);
  process_map[process->pid] = process;
}

void MemoryManager::unregister_process(int pid) {
  std::lock_guard<std::mutex> lock(mutex_);

  process_map.erase(pid);
  pending_pages_by_process.erase(pid);
  processes_waiting_on_memory.erase(pid);

  fault_queue.erase(std::remove_if(fault_queue.begin(), fault_queue.end(),
                                   [pid](const PageLoadTask &task) {
                                     return task.process &&
                                            task.process->pid == pid;
                                   }),
                    fault_queue.end());

  if (active_task && active_task->process && active_task->process->pid == pid) {
    active_task.reset();
  }

  for (auto &frame : frames) {
    if (frame.process_id == pid) {
      frame.process_id = -1;
      frame.page_id = -1;
      frame.occupied = false;
      if (algorithm)
        algorithm->on_frame_release(frame.frame_id);
    }
  }
}

void MemoryManager::set_ready_callback(ProcessReadyCallback callback) {
  std::lock_guard<std::mutex> lock(mutex_);
  ready_callback = std::move(callback);
}

bool MemoryManager::allocate_initial_memory(Process &process) {
  int num_pages = static_cast<int>(process.memory_required);
  process.page_table.resize(num_pages);
  for (int i = 0; i < num_pages; ++i) {
    process.page_table[i] = Page(i);
    process.page_table[i].process_id = process.pid;
  }
  return true;
}

bool MemoryManager::prepare_process_for_cpu(std::shared_ptr<Process> process,
                                            int current_time) {
  if (!process)
    return false;
  std::lock_guard<std::mutex> lock(mutex_);

  if (process->page_table.empty()) {
    allocate_initial_memory(*process);
  }

  if (are_all_pages_resident(*process)) {
    set_process_pages_referenced(*process, true);
    processes_waiting_on_memory.erase(process->pid);
    return true;
  }

  std::vector<int> missing_pages;
  auto &pending = pending_pages_by_process[process->pid];
  for (const auto &page : process->page_table) {
    if (!page.valid && pending.find(page.page_id) == pending.end()) {
      missing_pages.push_back(page.page_id);
    }
  }

  if (!missing_pages.empty()) {
    enqueue_missing_pages(process, missing_pages, current_time);
  }

  processes_waiting_on_memory.insert(process->pid);
  return false;
}

void MemoryManager::advance_fault_queue(int duration, int start_time) {
  if (duration <= 0)
    return;

  for (int step = 0; step < duration; ++step) {
    int tick_time = start_time + step;
    std::vector<std::shared_ptr<Process>> ready_processes;

    {
      std::lock_guard<std::mutex> lock(mutex_);
      memory_time = tick_time;

      if (!active_task) {
        start_next_task_if_possible(tick_time);
      }

      if (active_task) {
        active_task->remaining_time--;
        if (active_task->remaining_time <= 0) {
          auto newly_ready = complete_active_task(tick_time);
          if (newly_ready) {
            ready_processes.push_back(newly_ready);
          }
          active_task.reset();
        }
      }

      if (!active_task) {
        start_next_task_if_possible(tick_time);
      }
    }

    for (auto &proc : ready_processes) {
      if (ready_callback) {
        ready_callback(proc);
      }
    }
  }
}

void MemoryManager::mark_process_inactive(const Process &process) {
  std::lock_guard<std::mutex> lock(mutex_);
  set_process_pages_referenced(process, false);
}

void MemoryManager::release_process_memory(int pid) { unregister_process(pid); }

int MemoryManager::get_total_page_faults() const { return total_page_faults; }
int MemoryManager::get_total_replacements() const { return total_replacements; }

int MemoryManager::find_free_frame() {
  for (int i = 0; i < total_frames; ++i) {
    if (!frames[i].occupied)
      return i;
  }
  return -1;
}

bool MemoryManager::are_all_pages_resident(const Process &process) const {
  if (process.page_table.empty()) {
    return true;
  }

  for (const auto &page : process.page_table) {
    if (!page.valid)
      return false;
  }
  return true;
}

void MemoryManager::enqueue_missing_pages(std::shared_ptr<Process> process,
                                          const std::vector<int> &missing_pages,
                                          int current_time) {
  auto &pending = pending_pages_by_process[process->pid];
  for (int page_id : missing_pages) {
    pending.insert(page_id);
    PageLoadTask task{process, page_id, page_fault_latency, -1, current_time};
    fault_queue.push_back(task);
    process->page_faults++;
    total_page_faults++;
  }
}

void MemoryManager::start_next_task_if_possible(int current_time) {
  if (active_task || fault_queue.empty())
    return;

  auto task = fault_queue.front();
  if (!reserve_frame_for_task(task)) {
    return;
  }
  task.remaining_time = page_fault_latency;
  task.enqueue_time = current_time;
  active_task = task;
  fault_queue.pop_front();
}

bool MemoryManager::reserve_frame_for_task(PageLoadTask &task) {
  int frame_idx = find_free_frame();

  if (frame_idx == -1 && algorithm) {
    frame_idx = algorithm->select_victim(frames, process_map, memory_time);
    if (frame_idx != -1) {
      if (frame_idx < 0 || frame_idx >= total_frames)
        return false;

      auto &frame = frames[frame_idx];
      if (frame.occupied) {
        auto it = process_map.find(frame.process_id);
        if (it == process_map.end()) {
          evict_frame(frame_idx);
        } else {
          Process &victim_proc = *it->second;
          if (frame.page_id >= 0 &&
              frame.page_id < static_cast<int>(victim_proc.page_table.size())) {
            Page &victim_page = victim_proc.page_table[frame.page_id];
            if (victim_page.referenced) {
              return false;
            }
          }
          evict_frame(frame_idx);
        }
      }
    }
  }

  if (frame_idx == -1)
    return false;

  Frame &frame = frames[frame_idx];
  frame.occupied = true;
  frame.process_id = task.process ? task.process->pid : -1;
  frame.page_id = task.page_id;
  task.frame_id = frame_idx;
  return true;
}

void MemoryManager::evict_frame(int frame_idx) {
  if (frame_idx < 0 || frame_idx >= total_frames)
    return;
  Frame &frame = frames[frame_idx];

  if (frame.process_id != -1) {
    auto it = process_map.find(frame.process_id);
    if (it != process_map.end()) {
      Process &victim_proc = *it->second;
      if (frame.page_id >= 0 &&
          frame.page_id < static_cast<int>(victim_proc.page_table.size())) {
        Page &victim_page = victim_proc.page_table[frame.page_id];
        if (victim_page.valid) {
          victim_page.valid = false;
          victim_page.frame_number = -1;
          victim_proc.active_pages_count =
              std::max(0, victim_proc.active_pages_count - 1);
        }
      }
      victim_proc.replacements++;
      total_replacements++;
    }
  }

  if (algorithm) {
    algorithm->on_frame_release(frame_idx);
  }

  frame.process_id = -1;
  frame.page_id = -1;
  frame.occupied = false;
}

std::shared_ptr<Process>
MemoryManager::complete_active_task(int completion_time) {
  if (!active_task || !active_task->process)
    return nullptr;

  auto process = active_task->process;
  int pid = process->pid;
  int page_id = active_task->page_id;
  int frame_id = active_task->frame_id;

  auto pending_it = pending_pages_by_process.find(pid);
  if (pending_it != pending_pages_by_process.end()) {
    pending_it->second.erase(page_id);
    if (pending_it->second.empty()) {
      pending_pages_by_process.erase(pending_it);
    }
  }

  if (frame_id >= 0 && frame_id < total_frames) {
    Frame &frame = frames[frame_id];
    frame.process_id = pid;
    frame.page_id = page_id;
    frame.occupied = true;
  }

  if (page_id >= 0 && page_id < static_cast<int>(process->page_table.size())) {
    Page &page = process->page_table[page_id];
    page.valid = true;
    page.frame_number = frame_id;
    page.referenced = true;
    page.last_access_time = completion_time;
    process->active_pages_count++;
  }

  if (algorithm && frame_id >= 0) {
    algorithm->on_page_access(frame_id);
  }

  if (are_all_pages_resident(*process)) {
    processes_waiting_on_memory.erase(pid);
    set_process_pages_referenced(*process, true);
    return process;
  }

  return nullptr;
}

void MemoryManager::set_process_pages_referenced(const Process &process,
                                                 bool referenced) {
  auto it = process_map.find(process.pid);
  if (it == process_map.end())
    return;

  for (auto &page : it->second->page_table) {
    if (page.valid) {
      page.referenced = referenced;
    }
  }
}

std::string MemoryManager::generate_json_output() {
  std::lock_guard<std::mutex> lock(mutex_);
  
  std::string json = "{\n";
  json += "  \"memory_manager\": {\n";
  json += "    \"total_frames\": " + std::to_string(total_frames) + ",\n";
  json += "    \"total_page_faults\": " + std::to_string(total_page_faults) + ",\n";
  json += "    \"total_replacements\": " + std::to_string(total_replacements) + ",\n";
  json += "    \"page_fault_latency\": " + std::to_string(page_fault_latency) + ",\n";
  json += "    \"active_processes\": " + std::to_string(process_map.size()) + ",\n";
  json += "    \"processes_waiting_on_memory\": " + std::to_string(processes_waiting_on_memory.size()) + ",\n";
  json += "    \"pending_loads\": " + std::to_string(fault_queue.size()) + ",\n";
  
  // Frame state
  json += "    \"frames\": [\n";
  int free_frames = 0;
  for (size_t i = 0; i < frames.size(); ++i) {
    if (i > 0) json += ",\n";
    const Frame &f = frames[i];
    json += "      {\n";
    json += "        \"frame_id\": " + std::to_string(f.frame_id) + ",\n";
    json += "        \"occupied\": " + std::string(f.occupied ? "true" : "false") + ",\n";
    json += "        \"process_id\": " + std::to_string(f.process_id) + ",\n";
    json += "        \"page_id\": " + std::to_string(f.page_id) + "\n";
    json += "      }";
    if (!f.occupied) free_frames++;
  }
  json += "\n    ],\n";
  json += "    \"free_frames\": " + std::to_string(free_frames) + ",\n";
  
  // Page tables by process
  json += "    \"page_tables\": [\n";
  bool first_proc = true;
  for (const auto &[pid, proc_ptr] : process_map) {
    if (!first_proc) json += ",\n";
    first_proc = false;
    
    const Process &proc = *proc_ptr;
    json += "      {\n";
    json += "        \"pid\": " + std::to_string(proc.pid) + ",\n";
    json += "        \"process_name\": \"" + proc.name + "\",\n";
    json += "        \"page_faults\": " + std::to_string(proc.page_faults) + ",\n";
    json += "        \"replacements\": " + std::to_string(proc.replacements) + ",\n";
    json += "        \"memory_required\": " + std::to_string(proc.memory_required) + ",\n";
    json += "        \"pages\": [\n";
    
    for (size_t i = 0; i < proc.page_table.size(); ++i) {
      if (i > 0) json += ",\n";
      const Page &page = proc.page_table[i];
      json += "          {\n";
      json += "            \"page_id\": " + std::to_string(page.page_id) + ",\n";
      json += "            \"valid\": " + std::string(page.valid ? "true" : "false") + ",\n";
      json += "            \"frame_number\": " + std::to_string(page.frame_number) + ",\n";
      json += "            \"referenced\": " + std::string(page.referenced ? "true" : "false") + ",\n";
      json += "            \"modified\": " + std::string(page.modified ? "true" : "false") + ",\n";
      json += "            \"last_access_time\": " + std::to_string(page.last_access_time) + "\n";
      json += "          }";
    }
    
    json += "\n        ]\n";
    json += "      }";
  }
  json += "\n    ]\n";
  json += "  }\n";
  json += "}";
  
  return json;
}

bool MemoryManager::save_json_to_file(const std::string &filename) {
  std::string json_output = generate_json_output();
  
  std::ofstream file(filename);
  if (!file.is_open()) {
    std::cerr << "[MemoryManager] Error: Could not open file '" << filename << "' for writing.\n";
    return false;
  }
  
  file << json_output;
  file.close();
  
  return true;
}

} // namespace OSSimulator
