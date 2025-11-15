#ifndef SJF_SCHEDULER_HPP
#define SJF_SCHEDULER_HPP

#include "cpu/scheduler.hpp"
#include <vector>

namespace OSSimulator {

/**
 * Clase que implementa el algoritmo de planificación Shortest Job First (SJF).
 */
class SJFScheduler : public Scheduler {
private:
  std::vector<std::shared_ptr<Process>> ready_queue; //!< Cola de procesos listos.

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

#endif // SJF_SCHEDULER_HPP
