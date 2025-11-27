#include "io/io_device.hpp"
#include "metrics/metrics_collector.hpp"

namespace OSSimulator {

IODevice::IODevice(const std::string &name)
    : device_name(name), scheduler(nullptr), current_request(nullptr),
      total_io_time(0), device_switches(0), total_requests_completed(0),
      completion_callback(nullptr), metrics_collector(nullptr),
      last_event_was_completed(false) {}

void IODevice::set_scheduler(std::unique_ptr<IOScheduler> sched) {
  std::lock_guard<std::mutex> lock(device_mutex);
  scheduler = std::move(sched);
}

void IODevice::set_completion_callback(CompletionCallback callback) {
  completion_callback = callback;
}

void IODevice::set_metrics_collector(std::shared_ptr<MetricsCollector> collector) {
  std::lock_guard<std::mutex> lock(device_mutex);
  metrics_collector = collector;
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

  last_event_was_completed = current_request->is_completed();

  if (last_event_was_completed) {
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
  last_event_was_completed = false;
}

void IODevice::send_log_metrics(int current_time) {
  std::lock_guard<std::mutex> lock(device_mutex);

  if (!metrics_collector) {
    return;
  }

  std::string event;
  int pid = -1;
  std::string name;
  int remaining = 0;
  size_t queue_size = scheduler ? scheduler->size() : 0;

  if (!current_request && queue_size == 0) {
    event = "IDLE";

  } else if (last_event_was_completed) {
    event = "COMPLETED";
    if (current_request && current_request->process) {
      pid = current_request->process->pid;
      name = current_request->process->name;
    }

  } else if (current_request && current_request->process) {
    event = "STEP";
    pid = current_request->process->pid;
    name = current_request->process->name;
    remaining = current_request->burst.remaining_time;

  } else {
    event = "IDLE";
  }

  metrics_collector->log_io(
      current_time,
      device_name,
      event,
      pid,
      name,
      remaining,
      queue_size);
  
  last_event_was_completed = false;
}

std::string IODevice::get_device_status_json() const {
  std::lock_guard<std::mutex> lock(device_mutex);
  
  std::string json = "{\n";
  json += "  \"device_name\": \"" + device_name + "\",\n";
  json += "  \"is_busy\": " + std::string(current_request ? "true" : "false") + ",\n";
  json += "  \"queue_size\": " + std::to_string(scheduler ? scheduler->size() : 0) + ",\n";
  json += "  \"total_io_time\": " + std::to_string(total_io_time) + ",\n";
  json += "  \"device_switches\": " + std::to_string(device_switches) + ",\n";
  json += "  \"total_requests_completed\": " + std::to_string(total_requests_completed) + ",\n";
  
  if (scheduler) {
    std::string algo_name;
    switch (scheduler->get_algorithm()) {
      case IOSchedulingAlgorithm::FCFS:
        algo_name = "FCFS";
        break;
      case IOSchedulingAlgorithm::SJF:
        algo_name = "SJF";
        break;
      case IOSchedulingAlgorithm::ROUND_ROBIN:
        algo_name = "ROUND_ROBIN";
        break;
      case IOSchedulingAlgorithm::PRIORITY:
        algo_name = "PRIORITY";
        break;
      default:
        algo_name = "UNKNOWN";
    }
    json += "  \"algorithm\": \"" + algo_name + "\",\n";
  }
  
  json += "  \"current_request\": ";
  if (current_request && current_request->process) {
    json += "{\n";
    json += "    \"pid\": " + std::to_string(current_request->process->pid) + ",\n";
    json += "    \"process_name\": \"" + current_request->process->name + "\",\n";
    json += "    \"remaining_time\": " + std::to_string(current_request->burst.remaining_time) + ",\n";
    json += "    \"device\": \"" + current_request->burst.io_device + "\"\n";
    json += "  }\n";
  } else {
    json += "null\n";
  }
  
  json += "}";
  return json;
}


} // namespace OSSimulator
