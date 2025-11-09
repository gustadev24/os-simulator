#include "cpu/priority_scheduler.hpp"
#include <algorithm>

namespace OSSimulator {

void PriorityScheduler::add_process(const Process &process) {
  ready_queue.push_back(process);
  std::sort(ready_queue.begin(), ready_queue.end(),
            [](const Process &a, const Process &b) {
              if (a.priority == b.priority) {
                return a.arrival_time < b.arrival_time;
              }
              return a.priority < b.priority;
            });
}

Process *PriorityScheduler::get_next_process() {
  if (ready_queue.empty()) {
    return nullptr;
  }
  return &ready_queue.front();
}

bool PriorityScheduler::has_processes() const { return !ready_queue.empty(); }

void PriorityScheduler::remove_process(int pid) {
  for (auto it = ready_queue.begin(); it != ready_queue.end(); ++it) {
    if (it->pid == pid) {
      ready_queue.erase(it);
      return;
    }
  }
}

size_t PriorityScheduler::size() const { return ready_queue.size(); }

void PriorityScheduler::clear() { ready_queue.clear(); }

SchedulingAlgorithm PriorityScheduler::get_algorithm() const {
  return SchedulingAlgorithm::PRIORITY;
}

} // namespace OSSimulator
