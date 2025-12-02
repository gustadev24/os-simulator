#include "cpu/cpu_scheduler.hpp"
#include "cpu/fcfs_scheduler.hpp"
#include "cpu/round_robin_scheduler.hpp"
#include "metrics/metrics_collector.hpp"
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace OSSimulator;
using json = nlohmann::json;

TEST_CASE("Queue snapshot metrics are logged correctly", "[metrics][queues]") {
  std::filesystem::create_directories("data/test/resultados");
  const std::string path = "data/test/resultados/test_queue_snapshot.jsonl";

  std::filesystem::remove(path);

  auto metrics = std::make_shared<MetricsCollector>();
  REQUIRE(metrics->enable_file_output(path));

  auto scheduler = std::make_unique<FCFSScheduler>();
  CPUScheduler cpu_scheduler;
  cpu_scheduler.set_scheduler(std::move(scheduler));
  cpu_scheduler.set_metrics_collector(metrics);

  std::vector<std::shared_ptr<Process>> processes;
  processes.push_back(std::make_shared<Process>(
      1, "P1", 0, std::vector<Burst>{Burst(BurstType::CPU, 3)}));
  processes.push_back(std::make_shared<Process>(
      2, "P2", 1, std::vector<Burst>{Burst(BurstType::CPU, 2)}));

  cpu_scheduler.load_processes(processes);

  cpu_scheduler.execute_step(1);
  cpu_scheduler.execute_step(1);
  cpu_scheduler.execute_step(1);
  cpu_scheduler.execute_step(1);
  cpu_scheduler.execute_step(1);

  metrics->flush_all();
  metrics->disable_output();

  REQUIRE(std::filesystem::exists(path));

  std::ifstream infile(path);
  REQUIRE(infile.is_open());

  std::string line;
  bool found_queue_snapshot = false;

  while (std::getline(infile, line)) {
    if (line.empty())
      continue;
    auto j = json::parse(line);

    if (j.contains("queues")) {
      found_queue_snapshot = true;
      REQUIRE(j["queues"].contains("ready"));
      REQUIRE(j["queues"].contains("blocked_memory"));
      REQUIRE(j["queues"].contains("blocked_io"));
      REQUIRE(j["queues"].contains("running"));
      REQUIRE(j["queues"]["ready"].is_array());
      REQUIRE(j["queues"]["blocked_memory"].is_array());
      REQUIRE(j["queues"]["blocked_io"].is_array());
    }
  }

  REQUIRE(found_queue_snapshot);
}

TEST_CASE("Queue snapshots track process states correctly",
          "[metrics][queues]") {
  std::filesystem::create_directories("data/test/resultados");
  const std::string path = "data/test/resultados/test_queue_states.jsonl";

  std::filesystem::remove(path);

  auto metrics = std::make_shared<MetricsCollector>();
  REQUIRE(metrics->enable_file_output(path));

  auto scheduler = std::make_unique<RoundRobinScheduler>(2);
  CPUScheduler cpu_scheduler;
  cpu_scheduler.set_scheduler(std::move(scheduler));
  cpu_scheduler.set_metrics_collector(metrics);

  std::vector<std::shared_ptr<Process>> processes;
  processes.push_back(std::make_shared<Process>(
      1, "P1", 0, std::vector<Burst>{Burst(BurstType::CPU, 5)}));
  processes.push_back(std::make_shared<Process>(
      2, "P2", 0, std::vector<Burst>{Burst(BurstType::CPU, 3)}));

  cpu_scheduler.load_processes(processes);

  for (int i = 0; i < 8; i++) {
    cpu_scheduler.execute_step(1);
  }

  metrics->flush_all();
  metrics->disable_output();

  REQUIRE(std::filesystem::exists(path));

  std::ifstream infile(path);
  std::string line;
  int queue_snapshots_found = 0;

  while (std::getline(infile, line)) {
    if (line.empty())
      continue;
    auto j = json::parse(line);

    if (j.contains("queues")) {
      queue_snapshots_found++;
      int running_pid = j["queues"]["running"];
      REQUIRE((running_pid == -1 || running_pid == 1 || running_pid == 2));
    }
  }

  REQUIRE(queue_snapshots_found > 0);
}
