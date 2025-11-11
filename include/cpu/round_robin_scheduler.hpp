#ifndef ROUND_ROBIN_SCHEDULER_HPP
#define ROUND_ROBIN_SCHEDULER_HPP

#include "cpu/scheduler.hpp"
#include <deque>

namespace OSSimulator {

class RoundRobinScheduler : public Scheduler {
private:
  std::deque<Process*> ready_queue;
  int quantum;

public:
  explicit RoundRobinScheduler(int q = 4);

  void add_process(Process *process) override;
  Process *get_next_process() override;
  bool has_processes() const override;
  void remove_process(int pid) override;
  void rotate();
  size_t size() const override;
  void clear() override;
  int get_quantum() const;
  void set_quantum(int q);
  SchedulingAlgorithm get_algorithm() const override;
};

} // namespace OSSimulator

#endif // ROUND_ROBIN_SCHEDULER_HPP
