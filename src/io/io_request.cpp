#include "io/io_request.hpp"

namespace OSSimulator {

IORequest::IORequest()
    : process(nullptr), burst(), arrival_time(0), completion_time(0),
      start_time(-1), priority(0) {}

IORequest::IORequest(std::shared_ptr<Process> proc, const Burst &b,
                     int arrival, int prio)
    : process(proc), burst(b), arrival_time(arrival), completion_time(0),
      start_time(-1), priority(prio) {}

bool IORequest::is_completed() const { return burst.is_completed(); }

int IORequest::execute(int quantum, int current_time) {
  if (start_time < 0) {
    start_time = current_time;
  }

  int time_executed = (quantum > 0) ? std::min(quantum, burst.remaining_time)
                                    : burst.remaining_time;
  burst.remaining_time -= time_executed;

  if (is_completed()) {
    completion_time = current_time + time_executed;
  }

  return time_executed;
}

} // namespace OSSimulator
