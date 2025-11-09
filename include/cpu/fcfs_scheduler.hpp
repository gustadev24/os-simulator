#ifndef FCFS_SCHEDULER_HPP
#define FCFS_SCHEDULER_HPP

#include "cpu/scheduler.hpp"
#include <deque>

namespace OSSimulator {

class FCFSScheduler : public Scheduler {
private:
  std::deque<Process> ready_queue;

public:
  void add_process(const Process &process) override;
  Process *get_next_process() override;
  bool has_processes() const override;
  void remove_process(int pid) override;
  size_t size() const override;
  void clear() override;
  SchedulingAlgorithm get_algorithm() const override;
};

} // namespace OSSimulator

#endif // FCFS_SCHEDULER_HPP
