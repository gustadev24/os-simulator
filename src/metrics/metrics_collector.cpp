#include "metrics/metrics_collector.hpp"
#include <iostream>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace OSSimulator {

MetricsCollector::MetricsCollector()
    : file_out(nullptr), mode(OutputMode::DISABLED) {}

MetricsCollector::~MetricsCollector() { disable_output(); }

bool MetricsCollector::enable_file_output(const std::string &path) {
  std::lock_guard<std::mutex> lock(output_mutex);

  try {
    file_out =
        std::make_unique<std::ofstream>(path, std::ios::out | std::ios::app);
    if (!file_out->is_open()) {
      file_out.reset();
      mode = OutputMode::DISABLED;
      return false;
    }
    mode = OutputMode::FILE;
    return true;
  } catch (...) {
    file_out.reset();
    mode = OutputMode::DISABLED;
    return false;
  }
}

void MetricsCollector::enable_stdout_output() {
  std::lock_guard<std::mutex> lock(output_mutex);
  if (file_out && file_out->is_open()) {
    file_out->close();
  }
  file_out.reset();
  mode = OutputMode::STDOUT;
}

void MetricsCollector::disable_output() {
  flush_all();

  std::lock_guard<std::mutex> lock(output_mutex);
  if (file_out && file_out->is_open()) {
    file_out->close();
  }
  file_out.reset();
  mode = OutputMode::DISABLED;
}

void MetricsCollector::write_line(const std::string &json_line) {
  if (mode == OutputMode::DISABLED) {
    return;
  }

  if (mode == OutputMode::FILE && file_out) {
    (*file_out) << json_line << '\n';
    file_out->flush();
  } else if (mode == OutputMode::STDOUT) {
    std::cout << json_line << '\n';
    std::cout.flush();
  }
}

void MetricsCollector::flush_tick(int tick) {
  TickData data;
  bool has_data = false;

  {
    std::lock_guard<std::mutex> lock(output_mutex);

    auto it = tick_buffer.find(tick);
    if (it == tick_buffer.end())
      return;

    data = it->second;
    has_data = true;

    if (!data.has_cpu && !data.has_io) {
      tick_buffer.erase(it);
      return;
    }

    tick_buffer.erase(it);
    last_flushed_tick = tick;
  }

  if (!has_data)
    return;

  json j;
  j["tick"] = tick;

  if (data.has_cpu) {
    j["cpu"] = {{"event", data.cpu.event},
                {"pid", data.cpu.pid},
                {"name", data.cpu.name},
                {"remaining", data.cpu.remaining},
                {"ready_queue", data.cpu.ready_queue_size},
                {"context_switch", data.cpu.context_switch}};
  }

  if (data.has_io) {
    j["io"] = {{"device", data.io.device},
               {"event", data.io.event},
               {"pid", data.io.pid},
               {"name", data.io.name},
               {"remaining", data.io.remaining},
               {"queue", data.io.queue_size}};
  }

  write_line(j.dump());
}

void MetricsCollector::flush_all() {
  while (true) {
    int next_tick;
    {
      std::lock_guard<std::mutex> lock(output_mutex);

      if (tick_buffer.empty())
        break;

      next_tick = tick_buffer.begin()->first;
    }
    flush_tick(next_tick);
  }
}

void MetricsCollector::log_cpu(int tick, const std::string &event, int pid,
                               const std::string &name, int remaining,
                               size_t ready_queue_size,
                               bool context_switch_occurred) {
  std::lock_guard<std::mutex> lock(output_mutex);

  auto &t = tick_buffer[tick];
  t.cpu.event = event;
  t.cpu.pid = pid;
  t.cpu.name = name;
  t.cpu.remaining = remaining;
  t.cpu.ready_queue_size = ready_queue_size;
  t.cpu.context_switch = context_switch_occurred;
  t.has_cpu = true;
}

void MetricsCollector::log_io(int tick, const std::string &device_name,
                              const std::string &event, int pid,
                              const std::string &name, int remaining,
                              size_t queue_size) {
  std::lock_guard<std::mutex> lock(output_mutex);

  auto &t = tick_buffer[tick];
  t.io.device = device_name;
  t.io.event = event;
  t.io.pid = pid;
  t.io.name = name;
  t.io.remaining = remaining;
  t.io.queue_size = queue_size;
  t.has_io = true;
}

void MetricsCollector::log_cpu_summary(int total_time, double cpu_utilization,
                                       double avg_waiting_time,
                                       double avg_turnaround_time,
                                       double avg_response_time,
                                       int context_switches,
                                       const std::string &algorithm) {
  std::lock_guard<std::mutex> lock(output_mutex);

  if (mode == OutputMode::DISABLED) {
    return;
  }

  json j;
  j["summary"] = "CPU_METRICS";
  j["total_time"] = total_time;
  j["cpu_utilization"] = cpu_utilization;
  j["avg_waiting_time"] = avg_waiting_time;
  j["avg_turnaround_time"] = avg_turnaround_time;
  j["avg_response_time"] = avg_response_time;
  j["context_switches"] = context_switches;
  j["algorithm"] = algorithm;

  write_line(j.dump());
}

} // namespace OSSimulator
