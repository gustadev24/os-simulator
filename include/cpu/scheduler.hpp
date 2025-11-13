#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include "core/process.hpp"
#include <memory>

namespace OSSimulator {

enum class SchedulingAlgorithm { FCFS, SJF, ROUND_ROBIN, PRIORITY };

class Scheduler {
public:
  virtual ~Scheduler() = default;

  virtual void add_process(std::shared_ptr<Process> process) = 0;
  virtual std::shared_ptr<Process> get_next_process() = 0;
  virtual bool has_processes() const = 0;
  virtual void remove_process(int pid) = 0;
  virtual size_t size() const = 0;
  virtual void clear() = 0;
  virtual SchedulingAlgorithm get_algorithm() const = 0;
};

}

#endif // SCHEDULER_HPP
