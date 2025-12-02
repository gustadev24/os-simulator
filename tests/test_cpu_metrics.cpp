#include "core/process.hpp"
#include "cpu/cpu_scheduler.hpp"
#include "cpu/fcfs_scheduler.hpp"
#include "cpu/priority_scheduler.hpp"
#include "cpu/round_robin_scheduler.hpp"
#include "metrics/metrics_collector.hpp"
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace OSSimulator;
using json = nlohmann::json;

TEST_CASE("CPU Metrics - Basic logging", "[metrics][cpu]") {
  std::filesystem::create_directories("data/test/resultados");
  const std::string path = "data/test/resultados/test_cpu_basic_metrics.jsonl";

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
    REQUIRE(in.is_open());

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

  SECTION("Log CPU completion") {
    metrics->log_cpu(15, "COMPLETE", 3, "P3", 0, 0, false);
    metrics->flush_all();
    metrics->disable_output();

    std::ifstream in(path);
    std::string line;
    REQUIRE(std::getline(in, line));

    json j = json::parse(line);
    REQUIRE(j["cpu"]["event"] == "COMPLETE");
    REQUIRE(j["cpu"]["remaining"] == 0);
  }

  SECTION("Log CPU preemption") {
    metrics->log_cpu(20, "PREEMPT", 4, "P4", 5, 2, false);
    metrics->flush_all();
    metrics->disable_output();

    std::ifstream in(path);
    std::string line;
    REQUIRE(std::getline(in, line));

    json j = json::parse(line);
    REQUIRE(j["cpu"]["event"] == "PREEMPT");
  }
}

TEST_CASE("CPU Metrics - Summary statistics", "[metrics][cpu][summary]") {
  std::filesystem::create_directories("data/test/resultados");
  const std::string path = "data/test/resultados/test_cpu_summary.jsonl";

  if (std::filesystem::exists(path)) {
    std::filesystem::remove(path);
  }

  auto metrics = std::make_shared<MetricsCollector>();
  REQUIRE(metrics->enable_file_output(path));

  SECTION("Log CPU summary with FCFS") {
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

  SECTION("Log CPU summary with Round Robin") {
    metrics->log_cpu_summary(200, 92.3, 15.7, 35.8, 8.1, 12, "ROUND_ROBIN");
    metrics->flush_all();
    metrics->disable_output();

    std::ifstream in(path);
    std::string line;
    REQUIRE(std::getline(in, line));

    json j = json::parse(line);
    REQUIRE(j["algorithm"] == "ROUND_ROBIN");
    REQUIRE(j["context_switches"] == 12);
  }
}

TEST_CASE("CPU Metrics - Scheduler integration with FCFS",
          "[metrics][cpu][integration]") {
  std::filesystem::create_directories("data/test/resultados");
  const std::string path =
      "data/test/resultados/test_cpu_fcfs_integration.jsonl";

  if (std::filesystem::exists(path)) {
    std::filesystem::remove(path);
  }

  auto metrics = std::make_shared<MetricsCollector>();
  REQUIRE(metrics->enable_file_output(path));

  CPUScheduler scheduler;
  scheduler.set_scheduler(std::make_unique<FCFSScheduler>());
  scheduler.set_metrics_collector(metrics);

  std::vector<Burst> bursts_p1 = {Burst(BurstType::CPU, 3)};
  auto p1 = std::make_shared<Process>(1, "P1", 0, bursts_p1);

  std::vector<Burst> bursts_p2 = {Burst(BurstType::CPU, 2)};
  auto p2 = std::make_shared<Process>(2, "P2", 1, bursts_p2);

  std::vector<std::shared_ptr<Process>> processes = {p1, p2};
  scheduler.load_processes(processes);

  while (scheduler.has_pending_processes()) {
    scheduler.execute_step(1);
  }

  metrics->flush_all();

  metrics->log_cpu_summary(
      scheduler.get_current_time(), scheduler.get_cpu_utilization(),
      scheduler.get_average_waiting_time(),
      scheduler.get_average_turnaround_time(),
      scheduler.get_average_response_time(), scheduler.get_context_switches(),
      scheduler.get_algorithm_name());

  metrics->flush_all();
  metrics->disable_output();

  SECTION("File exists and contains events") {
    REQUIRE(std::filesystem::exists(path));

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
          REQUIRE(j["summary"] == "CPU_METRICS");
          REQUIRE(j["algorithm"] == "FCFS");
        } else if (j.contains("cpu")) {
          std::string event = j["cpu"]["event"];
          if (event == "EXEC") {
            exec_count++;
          } else if (event == "COMPLETE") {
            complete_count++;
          }
        }
      }
    }

    REQUIRE(exec_count > 0);
    REQUIRE(complete_count == 2);
    REQUIRE(has_summary);
  }

  SECTION("Verify process execution order") {
    std::ifstream in(path);
    std::string line;
    std::vector<int> execution_order;

    while (std::getline(in, line)) {
      if (!line.empty()) {
        json j = json::parse(line);

        if (j.contains("cpu") && j["cpu"]["event"] == "EXEC") {
          int pid = j["cpu"]["pid"];
          if (execution_order.empty() || execution_order.back() != pid) {
            execution_order.push_back(pid);
          }
        }
      }
    }

    REQUIRE(execution_order.size() >= 2);
    REQUIRE(execution_order[0] == 1);
    REQUIRE(execution_order[1] == 2);
  }
}

TEST_CASE("CPU Metrics - Scheduler integration with Round Robin",
          "[metrics][cpu][integration][rr]") {
  std::filesystem::create_directories("data/test/resultados");
  const std::string path = "data/test/resultados/test_cpu_rr_integration.jsonl";

  if (std::filesystem::exists(path)) {
    std::filesystem::remove(path);
  }

  auto metrics = std::make_shared<MetricsCollector>();
  REQUIRE(metrics->enable_file_output(path));

  CPUScheduler scheduler;
  int quantum = 2;
  scheduler.set_scheduler(std::make_unique<RoundRobinScheduler>(quantum));
  scheduler.set_metrics_collector(metrics);

  // Create processes with longer CPU bursts
  std::vector<Burst> bursts_p1 = {Burst(BurstType::CPU, 5)};
  auto p1 = std::make_shared<Process>(1, "P1", 0, bursts_p1);

  std::vector<Burst> bursts_p2 = {Burst(BurstType::CPU, 5)};
  auto p2 = std::make_shared<Process>(2, "P2", 0, bursts_p2);

  std::vector<std::shared_ptr<Process>> processes = {p1, p2};
  scheduler.load_processes(processes);

  // Run simulation
  while (scheduler.has_pending_processes()) {
    scheduler.execute_step(quantum);
  }

  metrics->flush_all();

  metrics->log_cpu_summary(
      scheduler.get_current_time(), scheduler.get_cpu_utilization(),
      scheduler.get_average_waiting_time(),
      scheduler.get_average_turnaround_time(),
      scheduler.get_average_response_time(), scheduler.get_context_switches(),
      scheduler.get_algorithm_name());

  metrics->flush_all();
  metrics->disable_output();

  SECTION("Round Robin preemption occurs") {
    std::ifstream in(path);
    std::string line;
    int preempt_count = 0;
    int context_switches_in_logs = 0;

    while (std::getline(in, line)) {
      if (!line.empty()) {
        json j = json::parse(line);

        if (j.contains("cpu")) {
          std::string event = j["cpu"]["event"];
          if (event == "PREEMPT") {
            preempt_count++;
          }

          if (j["cpu"]["context_switch"] == true) {
            context_switches_in_logs++;
          }
        }
      }
    }

    // With RR and quantum=2, we should see preemptions
    REQUIRE(preempt_count > 0);
    REQUIRE(context_switches_in_logs > 0);
  }

  SECTION("Algorithm is correctly logged") {
    std::ifstream in(path);
    std::string line;

    while (std::getline(in, line)) {
      if (!line.empty()) {
        json j = json::parse(line);

        if (j.contains("summary")) {
          REQUIRE(j["algorithm"] == "ROUND_ROBIN");
          break;
        }
      }
    }
  }
}

TEST_CASE("CPU Metrics - Ready queue tracking", "[metrics][cpu][queue]") {
  std::filesystem::create_directories("data/test/resultados");
  const std::string path = "data/test/resultados/test_cpu_queue.jsonl";

  if (std::filesystem::exists(path)) {
    std::filesystem::remove(path);
  }

  auto metrics = std::make_shared<MetricsCollector>();
  REQUIRE(metrics->enable_file_output(path));

  CPUScheduler scheduler;
  scheduler.set_scheduler(std::make_unique<FCFSScheduler>());
  scheduler.set_metrics_collector(metrics);

  // Create multiple processes arriving at same time
  std::vector<Burst> bursts_p1 = {Burst(BurstType::CPU, 2)};
  auto p1 = std::make_shared<Process>(1, "P1", 0, bursts_p1);

  std::vector<Burst> bursts_p2 = {Burst(BurstType::CPU, 2)};
  auto p2 = std::make_shared<Process>(2, "P2", 0, bursts_p2);

  std::vector<Burst> bursts_p3 = {Burst(BurstType::CPU, 2)};
  auto p3 = std::make_shared<Process>(3, "P3", 0, bursts_p3);

  std::vector<std::shared_ptr<Process>> processes = {p1, p2, p3};
  scheduler.load_processes(processes);

  // Run simulation
  while (scheduler.has_pending_processes()) {
    scheduler.execute_step(1);
  }

  metrics->flush_all();
  metrics->disable_output();

  SECTION("Ready queue size is tracked") {
    std::ifstream in(path);
    std::string line;
    bool found_non_zero_queue = false;

    while (std::getline(in, line)) {
      if (!line.empty()) {
        json j = json::parse(line);

        if (j.contains("cpu")) {
          size_t queue_size = j["cpu"]["ready_queue"];
          if (queue_size > 0) {
            found_non_zero_queue = true;
          }
        }
      }
    }

    // With 3 processes arriving at the same time, ready queue should have non-zero size
    REQUIRE(found_non_zero_queue);
  }
}

TEST_CASE("CPU Metrics - Context switch tracking", "[metrics][cpu][context]") {
  std::filesystem::create_directories("data/test/resultados");
  const std::string path = "data/test/resultados/test_cpu_context.jsonl";

  if (std::filesystem::exists(path)) {
    std::filesystem::remove(path);
  }

  auto metrics = std::make_shared<MetricsCollector>();
  REQUIRE(metrics->enable_file_output(path));

  CPUScheduler scheduler;
  scheduler.set_scheduler(std::make_unique<FCFSScheduler>());
  scheduler.set_metrics_collector(metrics);

  std::vector<Burst> bursts_p1 = {Burst(BurstType::CPU, 1)};
  auto p1 = std::make_shared<Process>(1, "P1", 0, bursts_p1);

  std::vector<Burst> bursts_p2 = {Burst(BurstType::CPU, 1)};
  auto p2 = std::make_shared<Process>(2, "P2", 0, bursts_p2);

  std::vector<std::shared_ptr<Process>> processes = {p1, p2};
  scheduler.load_processes(processes);

  while (scheduler.has_pending_processes()) {
    scheduler.execute_step(1);
  }

  metrics->flush_all();
  metrics->disable_output();

  SECTION("Context switches are logged") {
    std::ifstream in(path);
    std::string line;
    int context_switch_count = 0;

    while (std::getline(in, line)) {
      if (!line.empty()) {
        json j = json::parse(line);

        if (j.contains("cpu") && j["cpu"]["context_switch"] == true) {
          context_switch_count++;
        }
      }
    }

    // Should have at least 2 context switches (one for each process start)
    REQUIRE(context_switch_count >= 2);
  }
}

TEST_CASE("CPU Metrics - Utilization calculation",
          "[metrics][cpu][utilization]") {
  CPUScheduler scheduler;
  scheduler.set_scheduler(std::make_unique<FCFSScheduler>());

  std::vector<Burst> bursts_p1 = {Burst(BurstType::CPU, 5)};
  auto p1 = std::make_shared<Process>(1, "P1", 0, bursts_p1);

  std::vector<std::shared_ptr<Process>> processes = {p1};
  scheduler.load_processes(processes);

  while (scheduler.has_pending_processes()) {
    scheduler.execute_step(1);
  }

  SECTION("CPU utilization is calculated correctly") {
    double utilization = scheduler.get_cpu_utilization();

    // Single process running continuously should have 100% utilization
    REQUIRE(utilization > 99.0);
    REQUIRE(utilization <= 100.0);
  }
}

TEST_CASE("CPU Metrics - Algorithm name retrieval",
          "[metrics][cpu][algorithm]") {
  SECTION("FCFS algorithm name") {
    CPUScheduler scheduler;
    scheduler.set_scheduler(std::make_unique<FCFSScheduler>());
    REQUIRE(scheduler.get_algorithm_name() == "FCFS");
  }

  SECTION("Round Robin algorithm name") {
    CPUScheduler scheduler;
    scheduler.set_scheduler(std::make_unique<RoundRobinScheduler>(2));
    REQUIRE(scheduler.get_algorithm_name() == "ROUND_ROBIN");
  }

  SECTION("Priority algorithm name") {
    CPUScheduler scheduler;
    scheduler.set_scheduler(std::make_unique<PriorityScheduler>());
    REQUIRE(scheduler.get_algorithm_name() == "PRIORITY");
  }

  SECTION("No scheduler set") {
    CPUScheduler scheduler;
    REQUIRE(scheduler.get_algorithm_name() == "NONE");
  }
}
