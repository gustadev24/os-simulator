#include "cpu/round_robin_scheduler.hpp"

namespace OSSimulator {

RoundRobinScheduler::RoundRobinScheduler(int q) : quantum(q) {}

void RoundRobinScheduler::add_process(std::shared_ptr<Process> process) {
  ready_queue.push_back(process);
}

std::shared_ptr<Process> RoundRobinScheduler::get_next_process() {
  if (ready_queue.empty()) {
    return nullptr;
  }
  return ready_queue.front();
}

bool RoundRobinScheduler::has_processes() const { return !ready_queue.empty(); }

void RoundRobinScheduler::remove_process(int pid) {
  for (auto it = ready_queue.begin(); it != ready_queue.end(); ++it) {
    if ((*it)->pid == pid) {
      ready_queue.erase(it);
      return;
    }
  }
}

void RoundRobinScheduler::rotate() {
  if (!ready_queue.empty()) {
    ready_queue.push_back(ready_queue.front());
    ready_queue.pop_front();
  }
}

size_t RoundRobinScheduler::size() const { return ready_queue.size(); }

void RoundRobinScheduler::clear() { ready_queue.clear(); }

int RoundRobinScheduler::get_quantum() const { return quantum; }

void RoundRobinScheduler::set_quantum(int q) { quantum = q; }

SchedulingAlgorithm RoundRobinScheduler::get_algorithm() const {
  return SchedulingAlgorithm::ROUND_ROBIN;
}

} // namespace OSSimulator
