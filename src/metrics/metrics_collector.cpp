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

    if (!data.has_cpu && !data.has_io && !data.has_memory &&
        data.state_transitions.empty() && !data.has_queue_snapshot &&
        !data.has_page_table && !data.has_frame_status) {
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

  if (data.has_memory) {
    j["memory"] = {{"event", data.memory.event},
                   {"pid", data.memory.pid},
                   {"name", data.memory.name},
                   {"page_id", data.memory.page_id},
                   {"frame_id", data.memory.frame_id},
                   {"total_page_faults", data.memory.total_page_faults},
                   {"total_replacements", data.memory.total_replacements}};
  }

  if (!data.state_transitions.empty()) {
    json transitions = json::array();
    for (const auto &st : data.state_transitions) {
      transitions.push_back({{"pid", st.pid},
                             {"name", st.name},
                             {"from", st.from_state},
                             {"to", st.to_state},
                             {"reason", st.reason}});
    }
    j["state_transitions"] = transitions;
  }

  if (data.has_queue_snapshot) {
    j["queues"] = {{"ready", data.queue_snapshot.ready_queue},
                   {"blocked_memory", data.queue_snapshot.blocked_memory_queue},
                   {"blocked_io", data.queue_snapshot.blocked_io_queue},
                   {"running", data.queue_snapshot.running_pid}};
  }

  if (data.has_page_table) {
    json pages = json::array();
    for (const auto &entry : data.page_table.pages) {
      pages.push_back({{"page", entry.page_id},
                       {"frame", entry.frame_id},
                       {"valid", entry.valid},
                       {"referenced", entry.referenced},
                       {"modified", entry.modified}});
    }
    j["page_table"] = {{"pid", data.page_table.pid},
                       {"name", data.page_table.name},
                       {"pages", pages}};
  }

  if (data.has_frame_status) {
    json frames = json::array();
    for (const auto &entry : data.frame_status.frames) {
      frames.push_back({{"frame", entry.frame_id},
                        {"occupied", entry.occupied},
                        {"pid", entry.pid},
                        {"page", entry.page_id}});
    }
    j["frame_status"] = frames;
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

void MetricsCollector::log_memory(int tick, const std::string &event, int pid,
                                  const std::string &name, int page_id,
                                  int frame_id, int total_page_faults,
                                  int total_replacements) {
  std::lock_guard<std::mutex> lock(output_mutex);

  auto &t = tick_buffer[tick];
  t.memory.event = event;
  t.memory.pid = pid;
  t.memory.name = name;
  t.memory.page_id = page_id;
  t.memory.frame_id = frame_id;
  t.memory.total_page_faults = total_page_faults;
  t.memory.total_replacements = total_replacements;
  t.has_memory = true;
}

std::string MetricsCollector::process_state_to_string(ProcessState state) {
  switch (state) {
  case ProcessState::NEW:
    return "NEW";
  case ProcessState::READY:
    return "READY";
  case ProcessState::MEMORY_WAITING:
    return "MEMORY_WAITING";
  case ProcessState::RUNNING:
    return "RUNNING";
  case ProcessState::WAITING:
    return "WAITING";
  case ProcessState::TERMINATED:
    return "TERMINATED";
  default:
    return "UNKNOWN";
  }
}

void MetricsCollector::log_state_transition(int tick, int pid,
                                            const std::string &name,
                                            ProcessState from_state,
                                            ProcessState to_state,
                                            const std::string &reason) {
  std::lock_guard<std::mutex> lock(output_mutex);

  auto &t = tick_buffer[tick];
  StateTransitionData st;
  st.pid = pid;
  st.name = name;
  st.from_state = process_state_to_string(from_state);
  st.to_state = process_state_to_string(to_state);
  st.reason = reason;
  t.state_transitions.push_back(st);
}

void MetricsCollector::log_queue_snapshot(
    int tick, const std::vector<int> &ready_queue,
    const std::vector<int> &blocked_memory_queue,
    const std::vector<int> &blocked_io_queue, int running_pid) {
  std::lock_guard<std::mutex> lock(output_mutex);

  auto &t = tick_buffer[tick];
  t.queue_snapshot.ready_queue = ready_queue;
  t.queue_snapshot.blocked_memory_queue = blocked_memory_queue;
  t.queue_snapshot.blocked_io_queue = blocked_io_queue;
  t.queue_snapshot.running_pid = running_pid;
  t.has_queue_snapshot = true;
}

void MetricsCollector::log_page_table(
    int tick, int pid, const std::string &name,
    const std::vector<PageTableEntry> &page_table) {
  std::lock_guard<std::mutex> lock(output_mutex);

  auto &t = tick_buffer[tick];
  t.page_table.pid = pid;
  t.page_table.name = name;
  t.page_table.pages = page_table;
  t.has_page_table = true;
}

void MetricsCollector::log_frame_status(
    int tick, const std::vector<FrameStatusEntry> &frame_status) {
  std::lock_guard<std::mutex> lock(output_mutex);

  auto &t = tick_buffer[tick];
  t.frame_status.frames = frame_status;
  t.has_frame_status = true;
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

void MetricsCollector::log_memory_summary(int total_page_faults,
                                          int total_replacements,
                                          int total_frames, int used_frames,
                                          const std::string &algorithm) {
  std::lock_guard<std::mutex> lock(output_mutex);

  if (mode == OutputMode::DISABLED) {
    return;
  }

  json j;
  j["summary"] = "MEMORY_METRICS";
  j["total_page_faults"] = total_page_faults;
  j["total_replacements"] = total_replacements;
  j["total_frames"] = total_frames;
  j["used_frames"] = used_frames;
  j["frame_utilization"] =
      total_frames > 0 ? (100.0 * used_frames / total_frames) : 0.0;
  j["algorithm"] = algorithm;

  write_line(j.dump());
}

} // namespace OSSimulator
