#include "cpu/fcfs_scheduler.hpp"

namespace OSSimulator {

void FCFSScheduler::add_process(std::shared_ptr<Process> process) {
  ready_queue.push_back(process);
}

std::shared_ptr<Process> FCFSScheduler::get_next_process() {
  if (ready_queue.empty()) {
    return nullptr;
  }
  return ready_queue.front();
}

bool FCFSScheduler::has_processes() const { return !ready_queue.empty(); }

void FCFSScheduler::remove_process(int pid) {
  for (auto it = ready_queue.begin(); it != ready_queue.end(); ++it) {
    if ((*it)->pid == pid) {
      ready_queue.erase(it);
      return;
    }
  }
}

size_t FCFSScheduler::size() const { return ready_queue.size(); }

void FCFSScheduler::clear() { ready_queue.clear(); }

SchedulingAlgorithm FCFSScheduler::get_algorithm() const {
  return SchedulingAlgorithm::FCFS;
}

} // namespace OSSimulator
