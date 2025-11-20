#include "io/io_device.hpp"

namespace OSSimulator {

IODevice::IODevice(const std::string &name)
    : device_name(name), scheduler(nullptr), current_request(nullptr),
      total_io_time(0), device_switches(0), total_requests_completed(0),
      completion_callback(nullptr) {}

void IODevice::set_scheduler(std::unique_ptr<IOScheduler> sched) {
  std::lock_guard<std::mutex> lock(device_mutex);
  scheduler = std::move(sched);
}

void IODevice::set_completion_callback(CompletionCallback callback) {
  completion_callback = callback;
}

void IODevice::add_io_request(std::shared_ptr<IORequest> request) {
  std::lock_guard<std::mutex> lock(device_mutex);
  if (scheduler) {
    scheduler->add_request(request);
  }
}

void IODevice::execute_step(int quantum, int current_time) {
  std::lock_guard<std::mutex> lock(device_mutex);

  if (!scheduler) {
    return;
  }

  if (!scheduler->has_requests() && !current_request) {
    return;
  }

  if (!current_request) {
    current_request = scheduler->get_next_request();
    if (!current_request) {
      return;
    }
    device_switches++;
  }

  int time_executed = current_request->execute(quantum, current_time);
  total_io_time += time_executed;

  if (current_request->is_completed()) {
    total_requests_completed++;

    if (completion_callback && current_request->process) {
      completion_callback(current_request->process,
                          current_time + time_executed);
    }

    if (scheduler->get_algorithm() == IOSchedulingAlgorithm::ROUND_ROBIN) {
      // Request completed, don't re-add
    }

    current_request = nullptr;
  } else if (scheduler->get_algorithm() == IOSchedulingAlgorithm::ROUND_ROBIN) {
    scheduler->add_request(current_request);
    current_request = nullptr;
  }
}

bool IODevice::has_pending_requests() const {
  std::lock_guard<std::mutex> lock(device_mutex);
  return (scheduler && scheduler->has_requests()) ||
         (current_request != nullptr);
}

bool IODevice::is_busy() const {
  std::lock_guard<std::mutex> lock(device_mutex);
  return current_request != nullptr;
}

size_t IODevice::get_queue_size() const {
  std::lock_guard<std::mutex> lock(device_mutex);
  return scheduler ? scheduler->size() : 0;
}

void IODevice::reset() {
  std::lock_guard<std::mutex> lock(device_mutex);
  if (scheduler) {
    scheduler->clear();
  }
  current_request = nullptr;
  total_io_time = 0;
  device_switches = 0;
  total_requests_completed = 0;
}

} // namespace OSSimulator
