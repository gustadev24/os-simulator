#include "core/process.hpp"

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
  while (!should_terminate) {
    // Wait until state is RUNNING or should terminate
    {
      std::unique_lock<std::mutex> lock(process_mutex);
      state_cv.wait(lock, [this]() {
        return state == ProcessState::RUNNING || should_terminate;
      });

      // Exit if termination requested
      if (should_terminate) {
        break;
      }
    }

    // Simulate execution of one quantum
    // In a real simulation, this would be more sophisticated
    // For now, we just signal that we're ready to execute
    // The actual execution (remaining_time decrement) happens in execute()

    // Signal that this step is complete
    {
      std::lock_guard<std::mutex> lock(process_mutex);
      step_complete = true;
      state_cv.notify_all();
    }

    // Small sleep to simulate work (optional, helps with debugging)
    // std::this_thread::sleep_for(std::chrono::microseconds(100));
  }
}

} // namespace OSSimulator
