#include "memory/optimal_replacement.hpp"
#include "core/process.hpp"

namespace OSSimulator {

/**
 * Optimal replacement algorithm based on process state:
 * Priority order:
 * 1. Pages from TERMINATED processes (any of them)
 * 2. Pages from WAITING (I/O) processes, preferring those with longest remaining I/O time
 * 3. Any other unblocked page
 *
 * Pages with the `referenced` bit set are considered blocked (process is running)
 * and cannot be evicted.
 */
int OptimalReplacement::select_victim(
    const std::vector<Frame> &frames,
    const std::unordered_map<int, std::shared_ptr<Process>> &process_map,
    int /*current_time*/) {
  
  int victim_terminated = -1;
  int victim_io_waiting = -1;
  int longest_io_time = -1;
  int victim_unblocked = -1;

  for (const auto &frame : frames) {
    if (!frame.occupied)
      continue;

    auto it = process_map.find(frame.process_id);
    
    // If process is not in map, it was unregistered (terminated) - best candidate
    if (it == process_map.end()) {
      return frame.frame_id;
    }

    auto &proc = *it->second;

    // Check if page is referenced (blocked - process is running)
    if (frame.page_id >= 0 &&
        frame.page_id < static_cast<int>(proc.page_table.size())) {
      if (proc.page_table[frame.page_id].referenced) {
        // Page is blocked, cannot evict
        continue;
      }
    }

    ProcessState state = proc.state.load();

    // Priority 1: Terminated processes
    if (state == ProcessState::TERMINATED) {
      if (victim_terminated == -1) {
        victim_terminated = frame.frame_id;
      }
      continue;
    }

    // Priority 2: Waiting (I/O) processes - prefer longest remaining I/O time
    if (state == ProcessState::WAITING) {
      int remaining_io = 0;
      const Burst *current_burst = proc.get_current_burst();
      if (current_burst && current_burst->type == BurstType::IO) {
        remaining_io = current_burst->remaining_time;
      }

      if (remaining_io > longest_io_time) {
        longest_io_time = remaining_io;
        victim_io_waiting = frame.frame_id;
      }
      continue;
    }

    // Priority 3: Any other unblocked page (READY, MEMORY_WAITING, NEW)
    if (victim_unblocked == -1) {
      victim_unblocked = frame.frame_id;
    }
  }

  // Return based on priority order
  if (victim_terminated != -1) {
    return victim_terminated;
  }
  if (victim_io_waiting != -1) {
    return victim_io_waiting;
  }
  return victim_unblocked;
}

} // namespace OSSimulator
