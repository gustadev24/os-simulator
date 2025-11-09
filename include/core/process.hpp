#ifndef PROCESS_HPP
#define PROCESS_HPP

#include <cstdint>
#include <string>

namespace OSSimulator {

enum class ProcessState { NEW, READY, RUNNING, WAITING, TERMINATED };

struct Process {
  int pid;
  std::string name;
  int arrival_time;
  int burst_time;
  int remaining_time;
  int completion_time;
  int waiting_time;
  int turnaround_time;
  int response_time;
  int start_time;
  int priority;
  ProcessState state;
  bool first_execution;
  int last_execution_time;
  uint32_t memory_required;
  uint32_t memory_base;
  bool memory_allocated;

  Process();
  Process(int p, const std::string &n, int arrival, int burst, int prio = 0,
          uint32_t mem = 0);

  void calculate_metrics();
  bool has_arrived(int current_time) const;
  bool is_completed() const;
  int execute(int quantum, int current_time);
  void reset();
};

} // namespace OSSimulator

#endif // PROCESS_HPP
