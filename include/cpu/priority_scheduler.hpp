#ifndef PRIORITY_SCHEDULER_HPP
#define PRIORITY_SCHEDULER_HPP

#include "cpu/scheduler.hpp"
#include <vector>

namespace OSSimulator {

class PriorityScheduler : public Scheduler {
private:
  std::vector<Process> ready_queue;

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

#endif // PRIORITY_SCHEDULER_HPP
