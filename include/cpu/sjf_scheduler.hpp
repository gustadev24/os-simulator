#ifndef SJF_SCHEDULER_HPP
#define SJF_SCHEDULER_HPP

#include "cpu/scheduler.hpp"
#include <vector>

namespace OSSimulator {

class SJFScheduler : public Scheduler {
private:
  std::vector<Process*> ready_queue;

public:
  void add_process(Process *process) override;
  Process *get_next_process() override;
  bool has_processes() const override;
  void remove_process(int pid) override;
  size_t size() const override;
  void clear() override;
  SchedulingAlgorithm get_algorithm() const override;
};

} // namespace OSSimulator

#endif // SJF_SCHEDULER_HPP
