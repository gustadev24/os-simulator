#include "io/io_manager.hpp"
#include <iostream>

namespace OSSimulator {

void IOManager::add_device(const std::string &name,
                           std::shared_ptr<IODevice> device) {
  std::lock_guard<std::mutex> lock(manager_mutex);
  devices[name] = device;

  if (completion_callback) {
    device->set_completion_callback(completion_callback);
  }
  if (metrics_collector) {
    device->set_metrics_collector(metrics_collector);
  }
}

std::shared_ptr<IODevice> IOManager::get_device(const std::string &name) {
  std::lock_guard<std::mutex> lock(manager_mutex);
  auto it = devices.find(name);
  if (it != devices.end()) {
    return it->second;
  }
  return nullptr;
}

bool IOManager::has_device(const std::string &name) const {
  std::lock_guard<std::mutex> lock(manager_mutex);
  return devices.find(name) != devices.end();
}

void IOManager::set_completion_callback(CompletionCallback callback) {
  std::lock_guard<std::mutex> lock(manager_mutex);
  completion_callback = callback;

  for (auto &[name, device] : devices) {
    device->set_completion_callback(callback);
  }
}

void IOManager::set_metrics_collector(
    std::shared_ptr<MetricsCollector> collector) {
  std::lock_guard<std::mutex> lock(manager_mutex);
  metrics_collector = collector;

  for (auto &[name, device] : devices) {
    device->set_metrics_collector(collector);
  }
}

void IOManager::submit_io_request(std::shared_ptr<IORequest> request) {
  if (!request || !request->process) {
    return;
  }

  std::string device_name = request->burst.io_device;
  if (device_name.empty()) {
    device_name = "disk";
  }

  auto device = get_device(device_name);
  if (device) {
    device->add_io_request(request);
  } else {
    std::cerr << "[IOManager] Warning: Device '" << device_name
              << "' not found for process " << request->process->pid << "\n";
  }
}

void IOManager::execute_all_devices(int quantum, int current_time) {
  std::lock_guard<std::mutex> lock(manager_mutex);

  if (quantum <= 0) {
    for (auto &[name, device] : devices) {
      if (device->has_pending_requests()) {
        device->execute_step(0, current_time);
      }
      device->send_log_metrics(current_time);
    }
  } else {
    for (int tick = 0; tick < quantum; ++tick) {
      int tick_time = current_time + tick;
      for (auto &[name, device] : devices) {
        if (device->has_pending_requests()) {
          device->execute_step(1, tick_time);
        }
        device->send_log_metrics(tick_time);
      }
    }
  }
}

bool IOManager::has_pending_io() const {
  std::lock_guard<std::mutex> lock(manager_mutex);

  for (const auto &[name, device] : devices) {
    if (device->has_pending_requests()) {
      return true;
    }
  }

  return false;
}

void IOManager::reset_all_devices() {
  std::lock_guard<std::mutex> lock(manager_mutex);

  for (auto &[name, device] : devices) {
    device->reset();
  }
}

} // namespace OSSimulator
