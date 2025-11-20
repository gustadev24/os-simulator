#ifndef IO_REQUEST_HPP
#define IO_REQUEST_HPP

#include "core/burst.hpp"
#include "core/process.hpp"
#include <memory>

namespace OSSimulator {

struct IORequest {
  std::shared_ptr<Process> process;
  Burst burst;
  int arrival_time;
  int completion_time;
  int start_time;
  int priority;

  IORequest();
  IORequest(std::shared_ptr<Process> proc, const Burst &b, int arrival,
            int prio = 0);

  bool is_completed() const;
  int execute(int quantum, int current_time);
};

} // namespace OSSimulator

#endif // IO_REQUEST_HPP
