#ifndef CPU_SCHEDULER_HPP
#define CPU_SCHEDULER_HPP

#include "core/process.hpp"
#include "cpu/scheduler.hpp"
#include <functional>
#include <memory>
#include <vector>

namespace OSSimulator {

class CPUScheduler {
private:
  std::unique_ptr<Scheduler> scheduler;
  std::vector<Process> all_processes;
  std::vector<Process> completed_processes;
  int current_time;
  Process *running_process;
  int context_switches;

  using MemoryCheckCallback = std::function<bool(const Process &)>;
  MemoryCheckCallback memory_check_callback;

public:
  CPUScheduler();

  void set_scheduler(std::unique_ptr<Scheduler> sched);
  void set_memory_callback(MemoryCheckCallback callback);
  void add_process(const Process &process);
  void load_processes(const std::vector<Process> &processes);
  bool check_and_allocate_memory(Process &process);
  void add_arrived_processes();
  void execute_step(int quantum = 1);
  void run_until_completion();
  bool has_pending_processes() const;
  int get_current_time() const;
  int get_context_switches() const;
  const std::vector<Process> &get_completed_processes() const;
  const std::vector<Process> &get_all_processes() const;
  double get_average_waiting_time() const;
  double get_average_turnaround_time() const;
  double get_average_response_time() const;
  void reset();
};

} // namespace OSSimulator

#endif // CPU_SCHEDULER_HPP
