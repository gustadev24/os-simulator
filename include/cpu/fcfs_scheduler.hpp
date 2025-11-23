#ifndef FCFS_SCHEDULER_HPP
#define FCFS_SCHEDULER_HPP

#include "cpu/scheduler.hpp"
#include <deque>

namespace OSSimulator {

/**
 * Clase que implementa el algoritmo de planificación FCFS.
 */
class FCFSScheduler : public Scheduler {
private:
  std::deque<std::shared_ptr<Process>>
      ready_queue; //!< Cola de procesos listos.

public:
  void add_process(std::shared_ptr<Process> process) override;
  std::shared_ptr<Process> get_next_process() override;
  bool has_processes() const override;
  void remove_process(int pid) override;
  size_t size() const override;
  void clear() override;
  SchedulingAlgorithm get_algorithm() const override;
};

} // namespace OSSimulator

#endif // FCFS_SCHEDULER_HPP
