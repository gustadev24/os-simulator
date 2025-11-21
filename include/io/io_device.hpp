#ifndef IO_DEVICE_HPP
#define IO_DEVICE_HPP

#include "io/io_request.hpp"
#include "io/io_scheduler.hpp"
#include <functional>
#include <memory>
#include <mutex>
#include <string>

namespace OSSimulator {

class MetricsCollector;

class IODevice {
private:
  std::string device_name;
  std::unique_ptr<IOScheduler> scheduler;
  std::shared_ptr<IORequest> current_request;

  int total_io_time;
  int device_switches;
  int total_requests_completed;

  mutable std::mutex device_mutex;

  using CompletionCallback = std::function<void(std::shared_ptr<Process>, int)>;
  CompletionCallback completion_callback;

  std::shared_ptr<MetricsCollector> metrics_collector;
  bool last_event_was_completed;

public:
  explicit IODevice(const std::string &name);
  ~IODevice() = default;

  void set_scheduler(std::unique_ptr<IOScheduler> sched);
  void set_completion_callback(CompletionCallback callback);
  void set_metrics_collector(std::shared_ptr<MetricsCollector> collector);

  void add_io_request(std::shared_ptr<IORequest> request);

  void execute_step(int quantum, int current_time);

  bool has_pending_requests() const;
  bool is_busy() const;

  std::string get_device_name() const { return device_name; }
  int get_total_io_time() const { return total_io_time; }
  int get_device_switches() const { return device_switches; }
  int get_total_requests_completed() const { return total_requests_completed; }
  size_t get_queue_size() const;

  void send_log_metrics(int current_time);

  void reset();
};

} // namespace OSSimulator

#endif // IO_DEVICE_HPP
