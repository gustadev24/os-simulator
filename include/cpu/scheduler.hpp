#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include "core/process.hpp"

namespace OSSimulator {

enum class SchedulingAlgorithm { FCFS, SJF, ROUND_ROBIN, PRIORITY };

class Scheduler {
public:
  virtual ~Scheduler() = default;

  virtual void add_process(Process *process) = 0;
  virtual Process *get_next_process() = 0;
  virtual bool has_processes() const = 0;
  virtual void remove_process(int pid) = 0;
  virtual size_t size() const = 0;
  virtual void clear() = 0;
  virtual SchedulingAlgorithm get_algorithm() const = 0;
};

} // namespace OSSimulator

#endif // SCHEDULER_HPP
