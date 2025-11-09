#include "core/process.hpp"

namespace OSSimulator {

Process::Process()
    : pid(0), name(""), arrival_time(0), burst_time(0), remaining_time(0),
      completion_time(0), waiting_time(0), turnaround_time(0),
      response_time(-1), start_time(-1), priority(0), state(ProcessState::NEW),
      first_execution(true), last_execution_time(0), memory_required(0),
      memory_base(0), memory_allocated(false) {}

Process::Process(int p, const std::string &n, int arrival, int burst, int prio,
                 uint32_t mem)
    : pid(p), name(n), arrival_time(arrival), burst_time(burst),
      remaining_time(burst), completion_time(0), waiting_time(0),
      turnaround_time(0), response_time(-1), start_time(-1), priority(prio),
      state(ProcessState::NEW), first_execution(true), last_execution_time(0),
      memory_required(mem), memory_base(0), memory_allocated(false) {}

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

} // namespace OSSimulator
