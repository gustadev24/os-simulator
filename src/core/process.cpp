#include "core/process.hpp"
#include <numeric>

namespace OSSimulator {

Process::Process()
    : pid(0), name(""), arrival_time(0), burst_time(0), remaining_time(0),
      completion_time(0), waiting_time(0), turnaround_time(0),
      response_time(-1), start_time(-1), priority(0), state(ProcessState::NEW),
      first_execution(true), last_execution_time(0), memory_required(0),
      memory_base(0), memory_allocated(false), current_burst_index(0),
      total_cpu_time(0), total_io_time(0), process_thread(nullptr),
      should_terminate(false), step_complete(false) {}

Process::Process(int p, const std::string &n, int arrival, int burst, int prio,
                 uint32_t mem)
    : pid(p), name(n), arrival_time(arrival), burst_time(burst),
      remaining_time(burst), completion_time(0), waiting_time(0),
      turnaround_time(0), response_time(-1), start_time(-1), priority(prio),
      state(ProcessState::NEW), first_execution(true), last_execution_time(0),
      memory_required(mem), memory_base(0), memory_allocated(false),
      current_burst_index(0), total_cpu_time(0), total_io_time(0),
      process_thread(nullptr), should_terminate(false), step_complete(false) {
  burst_sequence.push_back(Burst(BurstType::CPU, burst));
  total_cpu_time = burst;
}

Process::Process(int p, const std::string &n, int arrival,
                 const std::vector<Burst> &bursts, int prio, uint32_t mem)
    : pid(p), name(n), arrival_time(arrival), burst_time(0), remaining_time(0),
      completion_time(0), waiting_time(0), turnaround_time(0),
      response_time(-1), start_time(-1), priority(prio),
      state(ProcessState::NEW), first_execution(true), last_execution_time(0),
      memory_required(mem), memory_base(0), memory_allocated(false),
      burst_sequence(bursts), current_burst_index(0), total_cpu_time(0),
      total_io_time(0), process_thread(nullptr), should_terminate(false),
      step_complete(false) {
  for (const auto &burst : burst_sequence) {
    if (burst.type == BurstType::CPU) {
      total_cpu_time += burst.duration;
    } else {
      total_io_time += burst.duration;
    }
  }
  burst_time = total_cpu_time;
  remaining_time = total_cpu_time;
}

Process::Process(Process &&other) noexcept
    : pid(other.pid), name(std::move(other.name)),
      arrival_time(other.arrival_time), burst_time(other.burst_time),
      remaining_time(other.remaining_time),
      completion_time(other.completion_time), waiting_time(other.waiting_time),
      turnaround_time(other.turnaround_time),
      response_time(other.response_time), start_time(other.start_time),
      priority(other.priority), state(other.state.load()),
      first_execution(other.first_execution),
      last_execution_time(other.last_execution_time),
      memory_required(other.memory_required), memory_base(other.memory_base),
      memory_allocated(other.memory_allocated),
      burst_sequence(std::move(other.burst_sequence)),
      current_burst_index(other.current_burst_index),
      total_cpu_time(other.total_cpu_time), total_io_time(other.total_io_time),
      process_thread(std::move(other.process_thread)),
      should_terminate(other.should_terminate.load()),
      step_complete(other.step_complete.load()) {}

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

bool Process::is_completed() const {
  return current_burst_index >= burst_sequence.size() &&
         (burst_sequence.empty() || remaining_time <= 0);
}

int Process::execute(int quantum, int current_time) {
  if (is_completed()) {
    return 0;
  }

  if (first_execution) {
    start_time = current_time;
    response_time = current_time - arrival_time;
    first_execution = false;
  }

  if (burst_sequence.empty()) {
    int time_executed =
        (quantum > 0) ? std::min(quantum, remaining_time) : remaining_time;
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

  if (current_burst_index >= burst_sequence.size()) {
    return 0;
  }

  Burst &current_burst = burst_sequence[current_burst_index];

  int time_executed = (quantum > 0)
                          ? std::min(quantum, current_burst.remaining_time)
                          : current_burst.remaining_time;

  {
    std::lock_guard<std::mutex> lock(process_mutex);
    current_burst.remaining_time -= time_executed;
    remaining_time -= time_executed;
    last_execution_time = current_time + time_executed;
  }

  if (current_burst.is_completed()) {
    std::lock_guard<std::mutex> lock(process_mutex);
    current_burst_index++;

    if (is_completed()) {
      completion_time = current_time + time_executed;
      state = ProcessState::TERMINATED;
      state_cv.notify_all();
    }
  }

  return time_executed;
}

void Process::reset() {
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
  current_burst_index = 0;

  for (auto &burst : burst_sequence) {
    burst.reset();
  }
}

void Process::start_thread() {
  stop_thread();
  should_terminate = false;
  step_complete = false;

  process_thread =
      std::make_unique<std::thread>(&Process::thread_function, this);
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
      return (state.load() == ProcessState::RUNNING && !step_complete.load()) ||
             should_terminate.load();
    });

    if (should_terminate.load())
      break;

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    step_complete.store(true);
    state_cv.notify_all();
  }
}

Process::~Process() { stop_thread(); }

bool Process::has_more_bursts() const {
  return current_burst_index < burst_sequence.size();
}

const Burst *Process::get_current_burst() const {
  if (current_burst_index >= burst_sequence.size()) {
    return nullptr;
  }
  return &burst_sequence[current_burst_index];
}

Burst *Process::get_current_burst_mutable() {
  if (current_burst_index >= burst_sequence.size()) {
    return nullptr;
  }
  return &burst_sequence[current_burst_index];
}

void Process::advance_to_next_burst() {
  if (current_burst_index < burst_sequence.size()) {
    current_burst_index++;
  }
}

bool Process::is_on_cpu_burst() const {
  const Burst *burst = get_current_burst();
  return burst && burst->type == BurstType::CPU;
}

bool Process::is_on_io_burst() const {
  const Burst *burst = get_current_burst();
  return burst && burst->type == BurstType::IO;
}

int Process::get_total_burst_time() const {
  return std::accumulate(
      burst_sequence.begin(), burst_sequence.end(), 0,
      [](int sum, const Burst &b) { return sum + b.duration; });
}

} // namespace OSSimulator
