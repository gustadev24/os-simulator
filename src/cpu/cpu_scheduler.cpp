#include "cpu/cpu_scheduler.hpp"
#include "cpu/round_robin_scheduler.hpp"
#include <chrono>
#include <thread>
#include <iostream>

namespace OSSimulator {

CPUScheduler::CPUScheduler()
    : scheduler(nullptr), current_time(0), running_process(nullptr),
      context_switches(0), memory_check_callback(nullptr),
      simulation_running(false) {
        all_processes.reserve(100);
      }

CPUScheduler::~CPUScheduler() {
  terminate_all_threads();
}

void CPUScheduler::set_scheduler(std::unique_ptr<Scheduler> sched) {
  scheduler = std::move(sched);
}

void CPUScheduler::set_memory_callback(MemoryCheckCallback callback) {
  memory_check_callback = callback;
}

void CPUScheduler::add_process(const Process &process) {
  all_processes.push_back(process);
  Process &added_proc = all_processes.back();
  spawn_process_thread(added_proc);
}

void CPUScheduler::load_processes(const std::vector<Process> &processes) {
  all_processes = processes;
  for (auto &proc : all_processes) {
    spawn_process_thread(proc);
  }
  completed_processes.clear();
  current_time = 0;
  context_switches = 0;
  running_process = nullptr;
  if (scheduler) scheduler->clear();
}

bool CPUScheduler::check_and_allocate_memory(Process &process) {
  if (memory_check_callback)
    return memory_check_callback(process);
  process.memory_allocated = true;
  return true;
}

void CPUScheduler::add_arrived_processes() {
  for (auto &proc : all_processes) {
    if (proc.state == ProcessState::NEW && proc.has_arrived(current_time)) {
      if (check_and_allocate_memory(proc)) {
        proc.state = ProcessState::READY;
        scheduler->add_process(&proc);
      }
    }
  }
}

void CPUScheduler::execute_step(int quantum) {
  std::lock_guard<std::mutex> scheduler_lock(scheduler_mutex);

  add_arrived_processes();

  if (!scheduler->has_processes()) {
    if (has_pending_processes()) current_time++;
    return;
  }

  std::cout << "Current time: " << current_time << "\n";
  Process *next = scheduler->get_next_process();
  if (!next) {
    current_time++;
    return;
  }
  std::cout << "Next process: " << next->pid << " (" << next->name << ")\n";

  if (running_process == nullptr || running_process->pid != next->pid)
    context_switches++;

  running_process = next;

  std::cout << "Running process: " << running_process->pid << " (" << running_process->name << ")\n";
  notify_process_running(running_process);
  wait_for_process_step(running_process);

  int time_executed = running_process->execute(quantum, current_time);
  current_time += time_executed;

  if (running_process->is_completed()) {
    running_process->calculate_metrics();
    running_process->stop_thread();

    completed_processes.push_back(*running_process);

    running_process->state = ProcessState::TERMINATED;

    scheduler->remove_process(running_process->pid);

    running_process = nullptr;
  } else if (scheduler->get_algorithm() == SchedulingAlgorithm::ROUND_ROBIN) {
    scheduler->remove_process(running_process->pid);
    scheduler->add_process(running_process);
  }
}

void CPUScheduler::run_until_completion() {
  std::cout << "[SCHEDULER] Iniciando simulación...\n";
  simulation_running = true;
  while (simulation_running && (has_pending_processes() || scheduler->has_processes())) {
    int quantum = 0;
    if (scheduler->get_algorithm() == SchedulingAlgorithm::ROUND_ROBIN) {
      if (auto *rr = dynamic_cast<RoundRobinScheduler *>(scheduler.get()))
        quantum = rr->get_quantum();
    }
    execute_step(quantum);
  }
}

bool CPUScheduler::has_pending_processes() const {
  for (const auto &proc : all_processes)
    if (proc.state != ProcessState::TERMINATED)
      return true;
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
  if (completed_processes.empty()) return 0.0;
  int total = 0;
  for (const auto &proc : completed_processes)
    total += proc.waiting_time;
  return static_cast<double>(total) / completed_processes.size();
}

double CPUScheduler::get_average_turnaround_time() const {
  if (completed_processes.empty()) return 0.0;
  int total = 0;
  for (const auto &proc : completed_processes)
    total += proc.turnaround_time;
  return static_cast<double>(total) / completed_processes.size();
}

double CPUScheduler::get_average_response_time() const {
  if (completed_processes.empty()) return 0.0;
  int total = 0;
  for (const auto &proc : completed_processes)
    total += proc.response_time;
  return static_cast<double>(total) / completed_processes.size();
}

void CPUScheduler::reset() {
  terminate_all_threads();
  for (auto &proc : all_processes) proc.reset();
  completed_processes.clear();
  current_time = 0;
  context_switches = 0;
  running_process = nullptr;
  if (scheduler) scheduler->clear();
}

void CPUScheduler::spawn_process_thread(Process &proc) {
  if (!proc.is_thread_running()) proc.start_thread();
}

void CPUScheduler::notify_process_running(Process *proc) {
  std::cout << "ENTRA A NOTIFICAR";
  if (!proc) return;
  std::lock_guard<std::mutex> lock(proc->process_mutex);

  std::cout << "[SCHEDULER] Notificando al proceso " << proc->pid << " para que ejecute -- RUNNING.\n";
  proc->state = ProcessState::RUNNING;
  proc->step_complete = false;
  proc->state_cv.notify_all();
}

void CPUScheduler::wait_for_process_step(Process *proc) {
  if (!proc) return;
  std::unique_lock<std::mutex> lock(proc->process_mutex);
  bool step_done = proc->state_cv.wait_for(
      lock, std::chrono::milliseconds(500),
      [proc]() { return proc->step_complete.load(); });

  if (!step_done) {
    std::cout << "[ERROR] [SCHEDULER] Timeout esperando el paso del proceso " << proc->pid << ". Terminando hilo.\n";
    proc->should_terminate = true;
    proc->state_cv.notify_all();
    return;
  }

  std::cout << "[NOTIFICADO] [SCHEDULER] Proceso " << proc->pid << " completó el paso.\n";

  proc->step_complete = false;
  if (proc->state == ProcessState::RUNNING) {
    proc->state = ProcessState::READY;

    std::cout << "[AVISANDO] [SCHEDULER] Proceso " << proc->pid << " cambiado a READY.\n";
    proc->state_cv.notify_all();
  }
}

void CPUScheduler::terminate_all_threads() {
  simulation_running = false;
  for (auto &proc : all_processes) {
    try {
      proc.stop_thread();
    } catch (...) {}
  }
}

} // namespace OSSimulator
