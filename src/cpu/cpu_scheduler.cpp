#include "cpu/cpu_scheduler.hpp"
#include "cpu/round_robin_scheduler.hpp"

namespace OSSimulator {

CPUScheduler::CPUScheduler()
    : scheduler(nullptr), current_time(0), running_process(nullptr),
      context_switches(0), memory_check_callback(nullptr),
      simulation_running(false) {}

CPUScheduler::~CPUScheduler() { terminate_all_threads(); }

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
        // Spawn thread BEFORE adding to scheduler
        spawn_process_thread(proc);

        proc.state = ProcessState::READY;
        scheduler->add_process(proc);
      }
    }
  }
}

void CPUScheduler::execute_step(int quantum) {
  // Protect the entire step with scheduler mutex
  std::lock_guard<std::mutex> scheduler_lock(scheduler_mutex);

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

  // Notify the process thread that it can run
  notify_process_running(running_process);

  // Wait for the process thread to complete one step
  wait_for_process_step(running_process);

  // Execute the actual work (decrement remaining_time, etc.)
  int time_executed = running_process->execute(quantum, current_time);
  current_time += time_executed;

  if (running_process->is_completed()) {
    running_process->calculate_metrics();

    // Stop the thread before moving to completed
    running_process->stop_thread();

    // Copy process data to completed_processes (no thread copied)
    Process completed_copy(running_process->pid, running_process->name,
                           running_process->arrival_time,
                           running_process->burst_time, running_process->priority,
                           running_process->memory_required);
    completed_copy.completion_time = running_process->completion_time;
    completed_copy.waiting_time = running_process->waiting_time;
    completed_copy.turnaround_time = running_process->turnaround_time;
    completed_copy.response_time = running_process->response_time;
    completed_copy.start_time = running_process->start_time;
    completed_copy.state = running_process->state.load();

    completed_processes.push_back(std::move(completed_copy));

    // Update state in all_processes (just update fields, don't copy threading)
    for (auto &proc : all_processes) {
      if (proc.pid == running_process->pid) {
        proc.state = ProcessState::TERMINATED;
        proc.completion_time = running_process->completion_time;
        proc.waiting_time = running_process->waiting_time;
        proc.turnaround_time = running_process->turnaround_time;
        proc.response_time = running_process->response_time;
        proc.start_time = running_process->start_time;
        proc.remaining_time = running_process->remaining_time;
        break;
      }
    }

    scheduler->remove_process(running_process->pid);
    running_process = nullptr;
  } else {
    running_process->state = ProcessState::READY;

    // Update in all_processes
    for (auto &proc : all_processes) {
      if (proc.pid == running_process->pid) {
        proc.remaining_time = running_process->remaining_time;
        proc.last_execution_time = running_process->last_execution_time;
        proc.state = running_process->state.load();
        if (running_process->start_time >= 0) {
          proc.start_time = running_process->start_time;
          proc.response_time = running_process->response_time;
          proc.first_execution = false;
        }
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
  // Terminate all threads first
  terminate_all_threads();

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

// Threading helper methods
void CPUScheduler::spawn_process_thread(Process &proc) {
  // Only spawn if thread doesn't exist
  if (!proc.is_thread_running()) {
    proc.start_thread();
  }
}

void CPUScheduler::notify_process_running(Process *proc) {
  if (!proc)
    return;

  std::lock_guard<std::mutex> lock(proc->process_mutex);
  proc->state = ProcessState::RUNNING;
  proc->step_complete = false;
  proc->state_cv.notify_all();
}

void CPUScheduler::wait_for_process_step(Process *proc) {
  if (!proc)
    return;

  std::unique_lock<std::mutex> lock(proc->process_mutex);
  proc->state_cv.wait(lock, [proc]() { return proc->step_complete.load(); });
  proc->step_complete = false;
}

void CPUScheduler::terminate_all_threads() {
  simulation_running = false;

  for (auto &proc : all_processes) {
    try {
      proc.stop_thread();
    } catch (const std::exception &e) {
      // Log error but continue terminating other threads
      // In production, you'd want proper logging here
    }
  }
}

} // namespace OSSimulator
