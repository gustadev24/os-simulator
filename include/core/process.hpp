#ifndef PROCESS_HPP
#define PROCESS_HPP

#include "core/burst.hpp"
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace OSSimulator {

enum class ProcessState { NEW, READY, RUNNING, WAITING, BLOCKED_IO, TERMINATED };

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
  std::atomic<ProcessState> state;
  bool first_execution;
  int last_execution_time;
  uint32_t memory_required;
  uint32_t memory_base;
  bool memory_allocated;

  // Burst sequence for CPU and I/O
  std::vector<Burst> burst_sequence;
  size_t current_burst_index;
  int total_cpu_time;
  int total_io_time;

  // Threading components
  std::unique_ptr<std::thread> process_thread;
  mutable std::mutex process_mutex;
  mutable std::condition_variable state_cv;
  std::atomic<bool> should_terminate;
  std::atomic<bool> step_complete;

  Process();
  Process(int p, const std::string &n, int arrival, int burst, int prio = 0,
          uint32_t mem = 0);

  Process(int p, const std::string &n, int arrival,
          const std::vector<Burst> &bursts, int prio = 0, uint32_t mem = 0);

  Process(const Process &other) = delete;
  Process &operator=(const Process &other) = delete;

  Process(Process &&other) noexcept;

  void calculate_metrics();
  bool has_arrived(int current_time) const;
  bool is_completed() const;
  int execute(int quantum, int current_time);
  void reset();

  // Burst sequence management
  bool has_more_bursts() const;
  const Burst *get_current_burst() const;
  Burst *get_current_burst_mutable();
  void advance_to_next_burst();
  bool is_on_cpu_burst() const;
  bool is_on_io_burst() const;
  int get_total_burst_time() const;

  ~Process();

  // Threading methods
  void start_thread();
  void stop_thread();
  bool is_thread_running() const;

private:
  void thread_function();
};

} // namespace OSSimulator

#endif // PROCESS_HPP
