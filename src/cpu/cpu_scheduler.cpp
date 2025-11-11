#include "cpu/cpu_scheduler.hpp"
#include "cpu/round_robin_scheduler.hpp"
#include <iostream>

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
  // Start the thread immediately when process is added
  Process &added_proc = all_processes.back();
  spawn_process_thread(added_proc);
  //   std::cout << "[Scheduler] Process " << added_proc.pid << " added and thread started" << std::endl;
}

void CPUScheduler::load_processes(const std::vector<Process> &processes) {
  all_processes = processes;
  // Start threads for all loaded processes
  for (auto &proc : all_processes) {
    spawn_process_thread(proc);
    // std::cout << "[Scheduler] Process " << proc.pid << " loaded and thread started" << std::endl;
  }
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
      // std::cout << "[Scheduler] Process " << proc.pid << " has arrived at time " << current_time << std::endl;
      if (check_and_allocate_memory(proc)) {
        // Thread was already spawned when process was added
        // std::cout << "[Scheduler] Process " << proc.pid << " state set to READY" << std::endl;

        proc.state = ProcessState::READY;
        scheduler->add_process(&proc);
      }
    }
  }
}

void CPUScheduler::execute_step(int quantum) {
  // Protect the entire step with scheduler mutex
  std::lock_guard<std::mutex> scheduler_lock(scheduler_mutex);

  //   std::cout << "[Scheduler] execute_step() at time " << current_time << std::endl;
  
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

  //   std::cout << "[Scheduler] Selected process " << next->pid << " to run" << std::endl;

  if (running_process == nullptr || running_process->pid != next->pid) {
    context_switches++;
  }

  running_process = next;
  
  //   std::cout << "[Scheduler] running_process pointer = " << running_process << " pid=" << running_process->pid << std::endl;
  
  // Find the REAL process in all_processes (the one with the thread)
  Process* real_process = nullptr;
  for (auto &proc : all_processes) {
    if (proc.pid == running_process->pid) {
      real_process = &proc;
      // std::cout << "[Scheduler] Found real_process in all_processes, pointer = " << real_process << std::endl;
      break;
    }
  }
  
  if (!real_process) {
    // std::cout << "[Scheduler] ERROR: Could not find real_process!" << std::endl;
    return;
  }

  // Notify the REAL process thread that it can run
  //   std::cout << "[Scheduler] About to notify real_process " << real_process->pid << std::endl;
  notify_process_running(real_process);

  // Wait for the REAL process thread to complete one step
  // (wait_for_process_step also changes state back to READY)
  //   std::cout << "[Scheduler] About to wait for real_process " << real_process->pid << std::endl;
  wait_for_process_step(real_process);
  
  //   std::cout << "[Scheduler] real_process " << real_process->pid << " completed step!" << std::endl;

  // Execute the actual work (decrement remaining_time, etc.)
  int time_executed = real_process->execute(quantum, current_time);
  current_time += time_executed;
  
  //   std::cout << "[Scheduler] Executed " << time_executed << " time units, remaining=" << real_process->remaining_time << std::endl;

  if (real_process->is_completed()) {
    // std::cout << "[Scheduler] Process " << real_process->pid << " is COMPLETED!" << std::endl;
    real_process->calculate_metrics();

    // Stop the thread before moving to completed
    // std::cout << "[Scheduler] Stopping thread for process " << real_process->pid << std::endl;
    real_process->stop_thread();

    // Copy process data to completed_processes (no thread copied)
    Process completed_copy(real_process->pid, real_process->name,
                           real_process->arrival_time,
                           real_process->burst_time, real_process->priority,
                           real_process->memory_required);
    completed_copy.completion_time = real_process->completion_time;
    completed_copy.waiting_time = real_process->waiting_time;
    completed_copy.turnaround_time = real_process->turnaround_time;
    completed_copy.response_time = real_process->response_time;
    completed_copy.start_time = real_process->start_time;
    completed_copy.state = real_process->state.load();

    completed_processes.push_back(std::move(completed_copy));

    // Mark as TERMINATED in all_processes (already updated since real_process points to it)
    real_process->state = ProcessState::TERMINATED;
    // std::cout << "[Scheduler] Marked process " << real_process->pid << " as TERMINATED" << std::endl;

    scheduler->remove_process(real_process->pid);
    running_process = nullptr;
    // std::cout << "[Scheduler] Removed process from scheduler, running_process = nullptr" << std::endl;
  } else {
    // std::cout << "[Scheduler] Process " << real_process->pid << " not completed, continuing..." << std::endl;
    
    // real_process already points to all_processes, so state is already updated
    // Update the pointer in scheduler if needed (Round Robin)
    if (scheduler->get_algorithm() == SchedulingAlgorithm::ROUND_ROBIN) {
      // std::cout << "[Scheduler] Round Robin - rotating queue" << std::endl;
      scheduler->remove_process(real_process->pid);
      scheduler->add_process(real_process);
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

  //   std::cout << "[Scheduler] Notifying process " << proc->pid << " to run, current state=" << (int)proc->state.load() << std::endl;
  std::lock_guard<std::mutex> lock(proc->process_mutex);
  proc->state = ProcessState::RUNNING;
  proc->step_complete = false;
  //   std::cout << "[Scheduler] Changed state to RUNNING (" << (int)ProcessState::RUNNING << "), notifying..." << std::endl;
  proc->state_cv.notify_all();
  //   std::cout << "[Scheduler] Notified process " << proc->pid << ", new state=" << (int)proc->state.load() << std::endl;
}

void CPUScheduler::wait_for_process_step(Process *proc) {
  if (!proc)
    return;

  //   std::cout << "[Scheduler] Waiting for process " << proc->pid << " to complete step..." << std::endl;
  std::unique_lock<std::mutex> lock(proc->process_mutex);
  proc->state_cv.wait(lock, [proc]() { return proc->step_complete.load(); });
  //   std::cout << "[Scheduler] Process " << proc->pid << " completed step!" << std::endl;
  
  // Reset flags and change state back to READY while holding the lock
  // This prevents the thread from executing again before we're done
  proc->step_complete = false;
  if (proc->state == ProcessState::RUNNING) {
    proc->state = ProcessState::READY;
    // std::cout << "[Scheduler] Changed state back to READY (inside lock), notifying thread..." << std::endl;
    // Notify the thread so it can see the state change
    proc->state_cv.notify_all();
  }
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
