#include "cpu/cpu_scheduler.hpp"
#include "cpu/round_robin_scheduler.hpp"

namespace OSSimulator {

CPUScheduler::CPUScheduler()
    : scheduler(nullptr), current_time(0), running_process(nullptr),
      context_switches(0), memory_check_callback(nullptr) {}

void CPUScheduler::set_scheduler(std::unique_ptr<Scheduler> sched) {
  scheduler = std::move(sched);
}

void CPUScheduler::set_memory_callback(MemoryCheckCallback callback) {
  memory_check_callback = callback;
}

void CPUScheduler::add_process(const Process &process) {
  all_processes.push_back(process);
}

void CPUScheduler::load_processes(const std::vector<Process> &processes) {
  all_processes = processes;
  completed_processes.clear();
  current_time = 0;
  context_switches = 0;
  running_process = nullptr;
  if (scheduler) {
    scheduler->clear();
  }
}

bool CPUScheduler::check_and_allocate_memory(Process &process) {
  if (memory_check_callback) {
    return memory_check_callback(process);
  }
  process.memory_allocated = true;
  return true;
}

void CPUScheduler::add_arrived_processes() {
  for (auto &proc : all_processes) {
    if (proc.state == ProcessState::NEW && proc.has_arrived(current_time)) {
      if (check_and_allocate_memory(proc)) {
        proc.state = ProcessState::READY;
        scheduler->add_process(proc);
      }
    }
  }
}

void CPUScheduler::execute_step(int quantum) {
  add_arrived_processes();

  if (!scheduler->has_processes()) {
    if (has_pending_processes()) {
      current_time++;
    }
    return;
  }

  Process *next = scheduler->get_next_process();
  if (!next) {
    current_time++;
    return;
  }

  if (running_process == nullptr || running_process->pid != next->pid) {
    context_switches++;
  }

  running_process = next;
  running_process->state = ProcessState::RUNNING;

  int time_executed = running_process->execute(quantum, current_time);
  current_time += time_executed;

  if (running_process->is_completed()) {
    running_process->calculate_metrics();
    completed_processes.push_back(*running_process);

    for (auto &proc : all_processes) {
      if (proc.pid == running_process->pid) {
        proc = *running_process;
        break;
      }
    }

    scheduler->remove_process(running_process->pid);
    running_process = nullptr;
  } else {
    running_process->state = ProcessState::READY;

    for (auto &proc : all_processes) {
      if (proc.pid == running_process->pid) {
        proc = *running_process;
        break;
      }
    }

    if (scheduler->get_algorithm() == SchedulingAlgorithm::ROUND_ROBIN) {
      scheduler->remove_process(running_process->pid);
      scheduler->add_process(*running_process);
    }
  }
}

void CPUScheduler::run_until_completion() {
  while (has_pending_processes() || scheduler->has_processes()) {
    int quantum = 1;
    if (scheduler->get_algorithm() == SchedulingAlgorithm::ROUND_ROBIN) {
      auto *rr = dynamic_cast<RoundRobinScheduler *>(scheduler.get());
      if (rr) {
        quantum = rr->get_quantum();
      }
    }
    execute_step(quantum);
  }
}

bool CPUScheduler::has_pending_processes() const {
  for (const auto &proc : all_processes) {
    if (proc.state != ProcessState::TERMINATED) {
      return true;
    }
  }
  return false;
}

int CPUScheduler::get_current_time() const { return current_time; }

int CPUScheduler::get_context_switches() const { return context_switches; }

const std::vector<Process> &CPUScheduler::get_completed_processes() const {
  return completed_processes;
}

const std::vector<Process> &CPUScheduler::get_all_processes() const {
  return all_processes;
}

double CPUScheduler::get_average_waiting_time() const {
  if (completed_processes.empty())
    return 0.0;
  int total = 0;
  for (const auto &proc : completed_processes) {
    total += proc.waiting_time;
  }
  return static_cast<double>(total) / completed_processes.size();
}

double CPUScheduler::get_average_turnaround_time() const {
  if (completed_processes.empty())
    return 0.0;
  int total = 0;
  for (const auto &proc : completed_processes) {
    total += proc.turnaround_time;
  }
  return static_cast<double>(total) / completed_processes.size();
}

double CPUScheduler::get_average_response_time() const {
  if (completed_processes.empty())
    return 0.0;
  int total = 0;
  for (const auto &proc : completed_processes) {
    total += proc.response_time;
  }
  return static_cast<double>(total) / completed_processes.size();
}

void CPUScheduler::reset() {
  for (auto &proc : all_processes) {
    proc.reset();
  }
  completed_processes.clear();
  current_time = 0;
  context_switches = 0;
  running_process = nullptr;
  if (scheduler) {
    scheduler->clear();
  }
}

} // namespace OSSimulator
