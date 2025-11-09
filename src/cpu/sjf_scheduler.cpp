#include "cpu/sjf_scheduler.hpp"
#include <algorithm>

namespace OSSimulator {

void SJFScheduler::add_process(const Process &process) {
  ready_queue.push_back(process);
  std::sort(ready_queue.begin(), ready_queue.end(),
            [](const Process &a, const Process &b) {
              if (a.remaining_time == b.remaining_time) {
                return a.arrival_time < b.arrival_time;
              }
              return a.remaining_time < b.remaining_time;
            });
}

Process *SJFScheduler::get_next_process() {
  if (ready_queue.empty()) {
    return nullptr;
  }
  return &ready_queue.front();
}

bool SJFScheduler::has_processes() const { return !ready_queue.empty(); }

void SJFScheduler::remove_process(int pid) {
  for (auto it = ready_queue.begin(); it != ready_queue.end(); ++it) {
    if (it->pid == pid) {
      ready_queue.erase(it);
      return;
    }
  }
}

size_t SJFScheduler::size() const { return ready_queue.size(); }

void SJFScheduler::clear() { ready_queue.clear(); }

SchedulingAlgorithm SJFScheduler::get_algorithm() const {
  return SchedulingAlgorithm::SJF;
}

} // namespace OSSimulator
