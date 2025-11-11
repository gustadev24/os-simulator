#include "core/process.hpp"
#include <iostream>

namespace OSSimulator {

Process::Process()
    : pid(0), name(""), arrival_time(0), burst_time(0), remaining_time(0),
      completion_time(0), waiting_time(0), turnaround_time(0),
      response_time(-1), start_time(-1), priority(0), state(ProcessState::NEW),
      first_execution(true), last_execution_time(0), memory_required(0),
      memory_base(0), memory_allocated(false), process_thread(nullptr),
      should_terminate(false), step_complete(false) {}

Process::Process(int p, const std::string &n, int arrival, int burst, int prio,
                 uint32_t mem)
    : pid(p), name(n), arrival_time(arrival), burst_time(burst),
      remaining_time(burst), completion_time(0), waiting_time(0),
      turnaround_time(0), response_time(-1), start_time(-1), priority(prio),
      state(ProcessState::NEW), first_execution(true), last_execution_time(0),
      memory_required(mem), memory_base(0), memory_allocated(false),
      process_thread(nullptr), should_terminate(false), step_complete(false) {}

// Copy constructor - copies all data but NOT the thread (thread is per-instance)
Process::Process(const Process &other)
    : pid(other.pid), name(other.name), arrival_time(other.arrival_time),
      burst_time(other.burst_time), remaining_time(other.remaining_time),
      completion_time(other.completion_time), waiting_time(other.waiting_time),
      turnaround_time(other.turnaround_time), response_time(other.response_time),
      start_time(other.start_time), priority(other.priority),
      state(other.state.load()), // Load the atomic value
      first_execution(other.first_execution),
      last_execution_time(other.last_execution_time),
      memory_required(other.memory_required), memory_base(other.memory_base),
      memory_allocated(other.memory_allocated),
      process_thread(nullptr), // Don't copy the thread
      should_terminate(other.should_terminate.load()),
      step_complete(other.step_complete.load()) {}

// Copy assignment operator
Process &Process::operator=(const Process &other) {
  if (this != &other) {
    // Stop any existing thread before copying
    stop_thread();

    pid = other.pid;
    name = other.name;
    arrival_time = other.arrival_time;
    burst_time = other.burst_time;
    remaining_time = other.remaining_time;
    completion_time = other.completion_time;
    waiting_time = other.waiting_time;
    turnaround_time = other.turnaround_time;
    response_time = other.response_time;
    start_time = other.start_time;
    priority = other.priority;
    state.store(other.state.load()); // Copy atomic value using load/store
    first_execution = other.first_execution;
    last_execution_time = other.last_execution_time;
    memory_required = other.memory_required;
    memory_base = other.memory_base;
    memory_allocated = other.memory_allocated;
    should_terminate.store(other.should_terminate.load());
    step_complete.store(other.step_complete.load());
    // process_thread remains nullptr (don't copy threads)
  }
  return *this;
}

void Process::calculate_metrics() {
  turnaround_time = completion_time - arrival_time;
  waiting_time = turnaround_time - burst_time;
  if (start_time >= 0) {
    response_time = start_time - arrival_time;
  }
}

bool Process::has_arrived(int current_time) const {
  return arrival_time <= current_time;
}

bool Process::is_completed() const { return remaining_time <= 0; }

int Process::execute(int quantum, int current_time) {
  if (first_execution) {
    start_time = current_time;
    response_time = current_time - arrival_time;
    first_execution = false;
  }

  int time_executed = (quantum < remaining_time) ? quantum : remaining_time;
  remaining_time -= time_executed;
  last_execution_time = current_time + time_executed;

  if (is_completed()) {
    completion_time = current_time + time_executed;
    state = ProcessState::TERMINATED;
  }

  return time_executed;
}

void Process::reset() {
  // Stop thread if running
  stop_thread();

  remaining_time = burst_time;
  completion_time = 0;
  waiting_time = 0;
  turnaround_time = 0;
  response_time = -1;
  start_time = -1;
  state = ProcessState::NEW;
  first_execution = true;
  last_execution_time = 0;
  memory_allocated = false;
  memory_base = 0;
}

// Threading implementations
void Process::start_thread() {
  // Don't start if already running
  if (process_thread && process_thread->joinable()) {
    return;
  }

  // Initialize threading flags
  should_terminate = false;
  step_complete = false;

  // Create the process thread
  process_thread = std::make_unique<std::thread>(&Process::thread_function, this);
}

void Process::stop_thread() {
  if (!process_thread) {
    return;
  }

  // Signal thread to terminate
  {
    std::lock_guard<std::mutex> lock(process_mutex);
    should_terminate = true;
    state_cv.notify_all();
  }

  // Wait for thread to finish
  if (process_thread->joinable()) {
    process_thread->join();
  }

  process_thread.reset();
}

bool Process::is_thread_running() const {
  return process_thread && process_thread->joinable();
}

void Process::thread_function() {
  while (!should_terminate.load()) {
    std::unique_lock<std::mutex> lock(process_mutex);
    
    // Wait until state is RUNNING or should terminate
    state_cv.wait(lock, [this]() {
      return state.load() == ProcessState::RUNNING || should_terminate.load();
    });
    
    // Exit if termination requested
    if (should_terminate.load()) {
      break;
    }

    // Simulate execution of one quantum
    // In a real simulation, this would be more sophisticated
    // For now, we just signal that we're ready to execute
    // The actual execution (remaining_time decrement) happens in execute()
    
    // Signal that this step is complete
    step_complete.store(true);
    state_cv.notify_all();
    
    // NOW wait until the scheduler changes our state back to NOT RUNNING
    // This prevents us from looping and executing multiple times
    state_cv.wait(lock, [this]() {
      return state.load() != ProcessState::RUNNING || should_terminate.load();
    });
  }
}

} // namespace OSSimulator
