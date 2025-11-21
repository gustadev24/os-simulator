#ifndef METRICS_COLLECTOR_HPP
#define METRICS_COLLECTOR_HPP

#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace OSSimulator {

class Process;
class IODevice;

class MetricsCollector {
public:
  enum class OutputMode {
    DISABLED,
    FILE,
    STDOUT
  };

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

  struct TickData {
    CpuTickData cpu;
    IoTickData io;
    bool has_cpu = false;
    bool has_io = false;
  };

  std::map<int, TickData> tick_buffer;
  int last_flushed_tick = -1;

  void write_line(const std::string &json_line);
  void flush_tick(int tick);

public:
  MetricsCollector();
  ~MetricsCollector();

  bool enable_file_output(const std::string &path);
  void enable_stdout_output();
  void disable_output();
  bool is_enabled() const { return mode != OutputMode::DISABLED; }
  
  void flush_all();

  void log_cpu(
      int tick,
      const std::string &event,
      int pid,
      const std::string &name,
      int remaining,
      size_t ready_queue_size,
      bool context_switch_occurred);
  
  void log_io(
      int tick,
      const std::string &device_name,
      const std::string &event,
      int pid,
      const std::string &name,
      int remaining,
      size_t queue_size);
};

}

#endif
