#include "core/process.hpp"
#include <iostream>

namespace OSSimulator {

Process::Process()
    : pid(0), name(""), arrival_time(0), burst_time(0), remaining_time(0),
      completion_time(0), waiting_time(0), turnaround_time(0),
      response_time(-1), start_time(-1), priority(0), state(ProcessState::NEW),
      first_execution(true), last_execution_time(0), memory_required(0),
      memory_base(0), memory_allocated(false), process_thread(nullptr),
      should_terminate(false), step_complete(false) { }

Process::Process(int p, const std::string &n, int arrival, int burst, int prio,
                 uint32_t mem)
    : pid(p), name(n), arrival_time(arrival), burst_time(burst),
      remaining_time(burst), completion_time(0), waiting_time(0),
      turnaround_time(0), response_time(-1), start_time(-1), priority(prio),
      state(ProcessState::NEW), first_execution(true), last_execution_time(0),
      memory_required(mem), memory_base(0), memory_allocated(false),
      process_thread(nullptr), should_terminate(false), step_complete(false) { }

Process::Process(Process &&other) noexcept
    : pid(other.pid), name(std::move(other.name)), arrival_time(other.arrival_time),
      burst_time(other.burst_time), remaining_time(other.remaining_time),
      completion_time(other.completion_time), waiting_time(other.waiting_time),
      turnaround_time(other.turnaround_time), response_time(other.response_time),
      start_time(other.start_time), priority(other.priority),
      state(other.state.load()), first_execution(other.first_execution),
      last_execution_time(other.last_execution_time),
      memory_required(other.memory_required), memory_base(other.memory_base),
      memory_allocated(other.memory_allocated),
      process_thread(std::move(other.process_thread)),
      should_terminate(other.should_terminate.load()),
      step_complete(other.step_complete.load()) { }

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

  int time_executed = (quantum > 0) ? std::min(quantum, remaining_time) : remaining_time;
  {
    std::lock_guard<std::mutex> lock(process_mutex);
    remaining_time -= time_executed;
    last_execution_time = current_time + time_executed;
  }

  if (is_completed()) {
    completion_time = current_time + time_executed;
    {
      std::lock_guard<std::mutex> lock(process_mutex);
      state = ProcessState::TERMINATED;
      state_cv.notify_all();
    }
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

void Process::start_thread() {
  stop_thread();
  should_terminate = false;
  step_complete = false;

  process_thread = std::make_unique<std::thread>(&Process::thread_function, this);
}

void Process::stop_thread() {
  if (!process_thread) {
    return;
  }

  {
    std::lock_guard<std::mutex> lock(process_mutex);
    should_terminate = true;
    state_cv.notify_all();
  }

  if (process_thread->joinable()) {
    process_thread->join();
  }

  process_thread.reset();
}

bool Process::is_thread_running() const {
  return process_thread && process_thread->joinable() && !should_terminate;
}

void Process::thread_function() {
  while (!should_terminate.load()) {
    std::unique_lock<std::mutex> lock(process_mutex);

    state_cv.wait(lock, [this]() {
      return state.load() == ProcessState::RUNNING || should_terminate.load();
    });

    if (should_terminate.load()) break;

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    step_complete.store(true);
    state_cv.notify_all();

    state_cv.wait(lock, [this]() {
      return state.load() != ProcessState::RUNNING || should_terminate.load();
    });
  }
}


Process::~Process() {
  stop_thread();
}

} // namespace OSSimulator
