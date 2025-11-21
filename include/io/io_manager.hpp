#ifndef IO_MANAGER_HPP
#define IO_MANAGER_HPP

#include "io/io_device.hpp"
#include "metrics/metrics_collector.hpp"
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace OSSimulator {

class IOManager {
private:
  std::map<std::string, std::shared_ptr<IODevice>> devices;
  mutable std::mutex manager_mutex;

  using CompletionCallback = std::function<void(std::shared_ptr<Process>, int)>;
  CompletionCallback completion_callback;

  std::shared_ptr<MetricsCollector> metrics_collector;

public:
  IOManager() = default;
  ~IOManager() = default;

  void add_device(const std::string &name, std::shared_ptr<IODevice> device);
  std::shared_ptr<IODevice> get_device(const std::string &name);
  bool has_device(const std::string &name) const;

  void set_completion_callback(CompletionCallback callback);

  void submit_io_request(std::shared_ptr<IORequest> request);

  void execute_all_devices(int quantum, int current_time);

  void set_metrics_collector(std::shared_ptr<MetricsCollector> collector);

  bool has_pending_io() const;

  void reset_all_devices();

  std::map<std::string, std::shared_ptr<IODevice>> get_all_devices() const {
    std::lock_guard<std::mutex> lock(manager_mutex);
    return devices;
  }
};

} // namespace OSSimulator

#endif // IO_MANAGER_HPP
