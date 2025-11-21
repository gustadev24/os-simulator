#include <catch2/catch_test_macros.hpp>
#include "io/io_manager.hpp"
#include "io/io_device.hpp"
#include "io/io_fcfs_scheduler.hpp"
#include "io/io_request.hpp"
#include "core/process.hpp"
#include "metrics/metrics_collector.hpp"
#include <filesystem>
#include <fstream>

using namespace OSSimulator;

TEST_CASE("IO JSONL output creates file and emits events", "[io][output]") {
  std::filesystem::create_directories("data/procesos");
  const std::string path = "data/procesos/io_events.jsonl";

  if (std::filesystem::exists(path)) {
    std::filesystem::remove(path);
  }

  auto metrics = std::make_shared<MetricsCollector>();
  REQUIRE(metrics->enable_file_output(path));

  IOManager manager;
  manager.set_metrics_collector(metrics);

  auto dev = std::make_shared<IODevice>("disk");
  dev->set_scheduler(std::make_unique<IOFCFSScheduler>());
  manager.add_device("disk", dev);

  auto p1 = std::make_shared<Process>(1, "P1", 0, 10);
  Burst b1(BurstType::IO, 3, "disk");
  auto r1 = std::make_shared<IORequest>(p1, b1, 0);

  auto p2 = std::make_shared<Process>(2, "P2", 1, 5);
  Burst b2(BurstType::IO, 2, "disk");
  auto r2 = std::make_shared<IORequest>(p2, b2, 1);

  manager.submit_io_request(r1);
  manager.submit_io_request(r2);

  int current_time = 0;
  int quantum = 1;

  while (manager.has_pending_io()) {
    manager.execute_all_devices(quantum, current_time);
    current_time += quantum;
  }

  metrics->disable_output();

  REQUIRE(std::filesystem::exists(path));

  std::ifstream in(path);
  REQUIRE(in.is_open());

  std::string line;
  int lines = 0;
  while (std::getline(in, line)) {
    if (!line.empty()) lines++;
  }

  REQUIRE(lines >= 1);
}
