/**
 * @file test_metrics.cpp
 * @brief Tests consolidados para el sistema de métricas
 */

#include "core/process.hpp"
#include "cpu/cpu_scheduler.hpp"
#include "cpu/fcfs_scheduler.hpp"
#include "cpu/priority_scheduler.hpp"
#include "cpu/round_robin_scheduler.hpp"
#include "io/io_device.hpp"
#include "io/io_fcfs_scheduler.hpp"
#include "io/io_manager.hpp"
#include "memory/fifo_replacement.hpp"
#include "memory/memory_manager.hpp"
#include "metrics/metrics_collector.hpp"
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace OSSimulator;
using json = nlohmann::json;

// ============================================================================
// INITIALIZATION TESTS
// ============================================================================

TEST_CASE("MetricsCollector - Initialization", "[metrics][init]") {
  SECTION("Default state is disabled") {
    MetricsCollector metrics;
    REQUIRE_FALSE(metrics.is_enabled());
  }

  SECTION("Enable file output successfully") {
    std::filesystem::create_directories("data/test/resultados");
    const std::string path = "data/test/resultados/test_init.jsonl";
    if (std::filesystem::exists(path)) {
      std::filesystem::remove(path);
    }

    MetricsCollector metrics;
    REQUIRE(metrics.enable_file_output(path));
    REQUIRE(metrics.is_enabled());

    metrics.disable_output();
    REQUIRE_FALSE(metrics.is_enabled());
  }

  SECTION("Enable stdout output") {
    MetricsCollector metrics;
    metrics.enable_stdout_output();
    REQUIRE(metrics.is_enabled());
    metrics.disable_output();
  }

  SECTION("Invalid path fails gracefully") {
    MetricsCollector metrics;
    REQUIRE_FALSE(metrics.enable_file_output("/invalid/path/to/file.jsonl"));
    REQUIRE_FALSE(metrics.is_enabled());
  }
}

// ============================================================================
// CPU METRICS TESTS
// ============================================================================

TEST_CASE("MetricsCollector - CPU Logging", "[metrics][cpu]") {
  std::filesystem::create_directories("data/test/resultados");
  const std::string path = "data/test/resultados/test_cpu_metrics.jsonl";

  if (std::filesystem::exists(path)) {
    std::filesystem::remove(path);
  }

  auto metrics = std::make_shared<MetricsCollector>();
  REQUIRE(metrics->enable_file_output(path));

  SECTION("Log CPU execution event") {
    metrics->log_cpu(0, "EXEC", 1, "P1", 10, 2, false);
    metrics->flush_all();
    metrics->disable_output();

    std::ifstream in(path);
    std::string line;
    REQUIRE(std::getline(in, line));

    json j = json::parse(line);
    REQUIRE(j["tick"] == 0);
    REQUIRE(j["cpu"]["event"] == "EXEC");
    REQUIRE(j["cpu"]["pid"] == 1);
    REQUIRE(j["cpu"]["name"] == "P1");
    REQUIRE(j["cpu"]["remaining"] == 10);
    REQUIRE(j["cpu"]["ready_queue"] == 2);
    REQUIRE(j["cpu"]["context_switch"] == false);
  }

  SECTION("Log CPU with context switch") {
    metrics->log_cpu(5, "EXEC", 2, "P2", 8, 1, true);
    metrics->flush_all();
    metrics->disable_output();

    std::ifstream in(path);
    std::string line;
    REQUIRE(std::getline(in, line));

    json j = json::parse(line);
    REQUIRE(j["cpu"]["context_switch"] == true);
  }

  SECTION("Log CPU idle state") {
    metrics->log_cpu(10, "IDLE", -1, "", 0, 0, false);
    metrics->flush_all();
    metrics->disable_output();

    std::ifstream in(path);
    std::string line;
    REQUIRE(std::getline(in, line));

    json j = json::parse(line);
    REQUIRE(j["cpu"]["event"] == "IDLE");
    REQUIRE(j["cpu"]["pid"] == -1);
  }
}

TEST_CASE("MetricsCollector - CPU Summary", "[metrics][cpu][summary]") {
  std::filesystem::create_directories("data/test/resultados");
  const std::string path = "data/test/resultados/test_cpu_summary.jsonl";

  if (std::filesystem::exists(path)) {
    std::filesystem::remove(path);
  }

  auto metrics = std::make_shared<MetricsCollector>();
  REQUIRE(metrics->enable_file_output(path));

  metrics->log_cpu_summary(100, 85.5, 10.5, 25.3, 5.2, 3, "FCFS");
  metrics->flush_all();
  metrics->disable_output();

  std::ifstream in(path);
  std::string line;
  REQUIRE(std::getline(in, line));

  json j = json::parse(line);
  REQUIRE(j["summary"] == "CPU_METRICS");
  REQUIRE(j["total_time"] == 100);
  REQUIRE(j["cpu_utilization"] == 85.5);
  REQUIRE(j["avg_waiting_time"] == 10.5);
  REQUIRE(j["avg_turnaround_time"] == 25.3);
  REQUIRE(j["avg_response_time"] == 5.2);
  REQUIRE(j["context_switches"] == 3);
  REQUIRE(j["algorithm"] == "FCFS");
}

TEST_CASE("MetricsCollector - CPU Scheduler Integration",
          "[metrics][cpu][integration]") {
  std::filesystem::create_directories("data/test/resultados");
  const std::string path = "data/test/resultados/test_cpu_integration.jsonl";

  if (std::filesystem::exists(path)) {
    std::filesystem::remove(path);
  }

  auto metrics = std::make_shared<MetricsCollector>();
  REQUIRE(metrics->enable_file_output(path));

  CPUScheduler scheduler;
  scheduler.set_scheduler(std::make_unique<FCFSScheduler>());
  scheduler.set_metrics_collector(metrics);

  auto p1 = std::make_shared<Process>(1, "P1", 0,
                                      std::vector<Burst>{Burst(BurstType::CPU, 3)});
  auto p2 = std::make_shared<Process>(2, "P2", 1,
                                      std::vector<Burst>{Burst(BurstType::CPU, 2)});

  scheduler.load_processes({p1, p2});

  while (scheduler.has_pending_processes()) {
    scheduler.execute_step(1);
  }

  metrics->log_cpu_summary(
      scheduler.get_current_time(), scheduler.get_cpu_utilization(),
      scheduler.get_average_waiting_time(),
      scheduler.get_average_turnaround_time(),
      scheduler.get_average_response_time(), scheduler.get_context_switches(),
      scheduler.get_algorithm_name());

  metrics->flush_all();
  metrics->disable_output();

  std::ifstream in(path);
  std::string line;
  int exec_count = 0;
  int complete_count = 0;
  bool has_summary = false;

  while (std::getline(in, line)) {
    if (!line.empty()) {
      json j = json::parse(line);
      if (j.contains("summary")) {
        has_summary = true;
        REQUIRE(j["algorithm"] == "FCFS");
      } else if (j.contains("cpu")) {
        std::string event = j["cpu"]["event"];
        if (event == "EXEC")
          exec_count++;
        if (event == "COMPLETE")
          complete_count++;
      }
    }
  }

  REQUIRE(exec_count > 0);
  REQUIRE(complete_count == 2);
  REQUIRE(has_summary);
}

// ============================================================================
// IO METRICS TESTS
// ============================================================================

TEST_CASE("MetricsCollector - IO Logging", "[metrics][io]") {
  std::filesystem::create_directories("data/test/resultados");
  const std::string path = "data/test/resultados/test_io_metrics.jsonl";

  if (std::filesystem::exists(path)) {
    std::filesystem::remove(path);
  }

  auto metrics = std::make_shared<MetricsCollector>();
  REQUIRE(metrics->enable_file_output(path));

  SECTION("Log IO start event") {
    metrics->log_io(0, "disk", "IO_START", 1, "P1", 5, 2);
    metrics->flush_all();
    metrics->disable_output();

    std::ifstream in(path);
    std::string line;
    REQUIRE(std::getline(in, line));

    json j = json::parse(line);
    REQUIRE(j["tick"] == 0);
    REQUIRE(j["io"]["device"] == "disk");
    REQUIRE(j["io"]["event"] == "IO_START");
    REQUIRE(j["io"]["pid"] == 1);
    REQUIRE(j["io"]["remaining"] == 5);
  }

  SECTION("Log IO completion") {
    metrics->log_io(10, "disk", "IO_COMPLETE", 3, "P3", 0, 0);
    metrics->flush_all();
    metrics->disable_output();

    std::ifstream in(path);
    std::string line;
    REQUIRE(std::getline(in, line));

    json j = json::parse(line);
    REQUIRE(j["io"]["event"] == "IO_COMPLETE");
    REQUIRE(j["io"]["remaining"] == 0);
  }
}

TEST_CASE("MetricsCollector - IO Manager Integration",
          "[metrics][io][integration]") {
  std::filesystem::create_directories("data/test/resultados");
  const std::string path = "data/test/resultados/test_io_integration.jsonl";

  if (std::filesystem::exists(path)) {
    std::filesystem::remove(path);
  }

  auto metrics = std::make_shared<MetricsCollector>();
  REQUIRE(metrics->enable_file_output(path));

  IOManager manager;
  manager.set_metrics_collector(metrics);

  auto device = std::make_shared<IODevice>("disk");
  device->set_scheduler(std::make_unique<IOFCFSScheduler>());
  manager.add_device("disk", device);

  auto p1 = std::make_shared<Process>(1, "P1", 0, 10);
  auto req = std::make_shared<IORequest>(p1, Burst(BurstType::IO, 3, "disk"), 0);

  manager.submit_io_request(req);

  int current_time = 0;
  while (manager.has_pending_io()) {
    manager.execute_all_devices(1, current_time);
    current_time++;
  }

  metrics->flush_all();
  metrics->disable_output();

  REQUIRE(std::filesystem::exists(path));
  std::ifstream in(path);
  std::string line;
  int io_events = 0;

  while (std::getline(in, line)) {
    if (!line.empty()) {
      json j = json::parse(line);
      if (j.contains("io")) {
        io_events++;
        REQUIRE(j["io"]["device"] == "disk");
      }
    }
  }

  REQUIRE(io_events > 0);
}

// ============================================================================
// MEMORY METRICS TESTS
// ============================================================================

TEST_CASE("MetricsCollector - Memory Logging", "[metrics][memory]") {
  std::filesystem::create_directories("data/test/resultados");
  const std::string path = "data/test/resultados/test_memory_metrics.jsonl";

  if (std::filesystem::exists(path)) {
    std::filesystem::remove(path);
  }

  auto metrics = std::make_shared<MetricsCollector>();
  REQUIRE(metrics->enable_file_output(path));

  SECTION("Log page fault") {
    metrics->log_memory(10, "PAGE_FAULT", 1, "P1", 3, -1, 5, 0);
    metrics->flush_all();
    metrics->disable_output();

    std::ifstream in(path);
    std::string line;
    REQUIRE(std::getline(in, line));

    json j = json::parse(line);
    REQUIRE(j["memory"]["event"] == "PAGE_FAULT");
    REQUIRE(j["memory"]["page_id"] == 3);
    REQUIRE(j["memory"]["total_page_faults"] == 5);
  }

  SECTION("Log page loaded") {
    metrics->log_memory(15, "PAGE_LOADED", 2, "P2", 5, 10, 8, 2);
    metrics->flush_all();
    metrics->disable_output();

    std::ifstream in(path);
    std::string line;
    REQUIRE(std::getline(in, line));

    json j = json::parse(line);
    REQUIRE(j["memory"]["event"] == "PAGE_LOADED");
    REQUIRE(j["memory"]["frame_id"] == 10);
  }
}

TEST_CASE("MetricsCollector - Memory Summary", "[metrics][memory][summary]") {
  std::filesystem::create_directories("data/test/resultados");
  const std::string path = "data/test/resultados/test_memory_summary.jsonl";

  if (std::filesystem::exists(path)) {
    std::filesystem::remove(path);
  }

  auto metrics = std::make_shared<MetricsCollector>();
  REQUIRE(metrics->enable_file_output(path));

  metrics->log_memory_summary(25, 10, 64, 48, "LRU");
  metrics->flush_all();
  metrics->disable_output();

  std::ifstream in(path);
  std::string line;
  REQUIRE(std::getline(in, line));

  json j = json::parse(line);
  REQUIRE(j["summary"] == "MEMORY_METRICS");
  REQUIRE(j["total_page_faults"] == 25);
  REQUIRE(j["total_replacements"] == 10);
  REQUIRE(j["frame_utilization"] == 75.0);
  REQUIRE(j["algorithm"] == "LRU");
}

TEST_CASE("MetricsCollector - Memory Manager Integration",
          "[metrics][memory][integration]") {
  std::filesystem::create_directories("data/test/resultados");
  const std::string path = "data/test/resultados/test_memory_mgr_integration.jsonl";

  if (std::filesystem::exists(path)) {
    std::filesystem::remove(path);
  }

  auto metrics = std::make_shared<MetricsCollector>();
  REQUIRE(metrics->enable_file_output(path));

  auto mm = std::make_shared<MemoryManager>(4, std::make_unique<FIFOReplacement>(), 1);
  mm->set_metrics_collector(metrics);

  auto p1 = std::make_shared<Process>(1, "P1", 0, 10, 0, 3);
  mm->register_process(p1);
  mm->allocate_initial_memory(*p1);
  mm->prepare_process_for_cpu(p1, 0);

  metrics->flush_all();
  mm->set_metrics_collector(nullptr);
  metrics->disable_output();

  std::ifstream in(path);
  bool found_event = false;
  std::string line;

  while (std::getline(in, line)) {
    if (!line.empty()) {
      json j = json::parse(line);
      if (j.contains("memory")) {
        found_event = true;
        REQUIRE(j["memory"]["pid"] == 1);
        break;
      }
    }
  }

  REQUIRE(found_event);
}

// ============================================================================
// COMBINED METRICS TESTS
// ============================================================================

TEST_CASE("MetricsCollector - Combined CPU and IO", "[metrics][combined]") {
  std::filesystem::create_directories("data/test/resultados");
  const std::string path = "data/test/resultados/test_combined.jsonl";

  if (std::filesystem::exists(path)) {
    std::filesystem::remove(path);
  }

  auto metrics = std::make_shared<MetricsCollector>();
  REQUIRE(metrics->enable_file_output(path));

  metrics->log_cpu(0, "EXEC", 1, "P1", 10, 2, false);
  metrics->log_io(0, "disk", "IO_START", 2, "P2", 5, 1);
  metrics->flush_all();
  metrics->disable_output();

  std::ifstream in(path);
  std::string line;
  REQUIRE(std::getline(in, line));

  json j = json::parse(line);
  REQUIRE(j["tick"] == 0);
  REQUIRE(j.contains("cpu"));
  REQUIRE(j.contains("io"));
  REQUIRE(j["cpu"]["pid"] == 1);
  REQUIRE(j["io"]["pid"] == 2);
}

TEST_CASE("MetricsCollector - Tick Ordering", "[metrics][ordering]") {
  std::filesystem::create_directories("data/test/resultados");
  const std::string path = "data/test/resultados/test_ordering.jsonl";

  if (std::filesystem::exists(path)) {
    std::filesystem::remove(path);
  }

  auto metrics = std::make_shared<MetricsCollector>();
  REQUIRE(metrics->enable_file_output(path));

  // Log out of order
  metrics->log_cpu(2, "EXEC", 3, "P3", 5, 0, false);
  metrics->log_cpu(0, "EXEC", 1, "P1", 10, 2, false);
  metrics->log_cpu(1, "EXEC", 2, "P2", 8, 1, false);

  metrics->flush_all();
  metrics->disable_output();

  std::ifstream in(path);
  std::string line;

  REQUIRE(std::getline(in, line));
  json j0 = json::parse(line);
  REQUIRE(j0["tick"] == 0);

  REQUIRE(std::getline(in, line));
  json j1 = json::parse(line);
  REQUIRE(j1["tick"] == 1);

  REQUIRE(std::getline(in, line));
  json j2 = json::parse(line);
  REQUIRE(j2["tick"] == 2);
}
