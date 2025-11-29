#ifndef METRICS_COLLECTOR_HPP
#define METRICS_COLLECTOR_HPP

#include "core/process.hpp"
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace OSSimulator {

class MetricsCollector {
public:
  enum class OutputMode { DISABLED, FILE, STDOUT };

private:
  mutable std::mutex output_mutex;
  std::unique_ptr<std::ofstream> file_out;
  OutputMode mode;

  struct CpuTickData {
    std::string event;
    int pid = -1;
    std::string name;
    int remaining = 0;
    size_t ready_queue_size = 0;
    bool context_switch = false;
  };

  struct IoTickData {
    std::string device;
    std::string event;
    int pid = -1;
    std::string name;
    int remaining = 0;
    size_t queue_size = 0;
  };

  struct MemoryTickData {
    std::string event;
    int pid = -1;
    std::string name;
    int page_id = -1;
    int frame_id = -1;
    int total_page_faults = 0;
    int total_replacements = 0;
  };

  struct StateTransitionData {
    int pid = -1;
    std::string name;
    std::string from_state;
    std::string to_state;
    std::string reason;
  };

  struct TickData {
    CpuTickData cpu;
    IoTickData io;
    MemoryTickData memory;
    StateTransitionData state_transition;
    bool has_cpu = false;
    bool has_io = false;
    bool has_memory = false;
    bool has_state_transition = false;
  };

  std::map<int, TickData> tick_buffer;
  int last_flushed_tick = -1;

  void write_line(const std::string &json_line);
  void flush_tick(int tick);

  static std::string process_state_to_string(ProcessState state);

public:
  MetricsCollector();
  ~MetricsCollector();

  bool enable_file_output(const std::string &path);
  void enable_stdout_output();
  void disable_output();
  bool is_enabled() const { return mode != OutputMode::DISABLED; }

  void flush_all();

  void log_cpu(int tick, const std::string &event, int pid,
               const std::string &name, int remaining, size_t ready_queue_size,
               bool context_switch_occurred);

  void log_io(int tick, const std::string &device_name,
              const std::string &event, int pid, const std::string &name,
              int remaining, size_t queue_size);

  void log_memory(int tick, const std::string &event, int pid,
                  const std::string &name, int page_id, int frame_id,
                  int total_page_faults, int total_replacements);

  void log_state_transition(int tick, int pid, const std::string &name,
                            ProcessState from_state, ProcessState to_state,
                            const std::string &reason);

  void log_cpu_summary(int total_time, double cpu_utilization,
                       double avg_waiting_time, double avg_turnaround_time,
                       double avg_response_time, int context_switches,
                       const std::string &algorithm);

  void log_memory_summary(int total_page_faults, int total_replacements,
                          int total_frames, int used_frames,
                          const std::string &algorithm);
};

} // namespace OSSimulator

#endif
