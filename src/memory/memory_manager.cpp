#include "memory/memory_manager.hpp"
#include "core/process.hpp"
#include "metrics/metrics_collector.hpp"
#include <algorithm>
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

void MemoryManager::set_metrics_collector(
    std::shared_ptr<MetricsCollector> collector) {
  std::lock_guard<std::mutex> lock(mutex_);
  metrics_collector = collector;
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

    if (metrics_collector && metrics_collector->is_enabled()) {
      metrics_collector->log_memory(current_time, "PAGE_FAULT", process->pid,
                                    process->name, page_id, -1,
                                    total_page_faults, total_replacements);
    }
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
        // Check if the victim page is referenced (second-chance mechanism)
        auto it = process_map.find(frame.process_id);
        if (it != process_map.end()) {
          Process &victim_proc = *it->second;
          if (frame.page_id >= 0 &&
              frame.page_id < static_cast<int>(victim_proc.page_table.size())) {
            Page &victim_page = victim_proc.page_table[frame.page_id];
            if (victim_page.referenced) {
              // Cannot evict referenced page, task stays in queue
              return false;
            }
          }
        }
        evict_frame(frame_idx);
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

  int evicted_pid = -1;
  std::string evicted_name;
  int evicted_page_id = frame.page_id;

  if (frame.process_id != -1) {
    auto it = process_map.find(frame.process_id);
    if (it != process_map.end()) {
      Process &victim_proc = *it->second;
      evicted_pid = victim_proc.pid;
      evicted_name = victim_proc.name;

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

      if (metrics_collector && metrics_collector->is_enabled()) {
        metrics_collector->log_memory(memory_time, "PAGE_REPLACED",
                                      evicted_pid, evicted_name,
                                      evicted_page_id, frame_idx,
                                      total_page_faults, total_replacements);
      }
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

  if (metrics_collector && metrics_collector->is_enabled()) {
    metrics_collector->log_memory(completion_time, "PAGE_LOADED", pid,
                                  process->name, page_id, frame_id,
                                  total_page_faults, total_replacements);
  }

  // Check if this process has no more pending pages
  auto pending_it_check = pending_pages_by_process.find(pid);
  bool no_pending_pages = (pending_it_check == pending_pages_by_process.end() || 
                           pending_it_check->second.empty());
  
  if (no_pending_pages && processes_waiting_on_memory.count(pid) > 0) {
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

} // namespace OSSimulator
