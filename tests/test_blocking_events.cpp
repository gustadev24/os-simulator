#include "cpu/cpu_scheduler.hpp"
#include "cpu/fcfs_scheduler.hpp"
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

TEST_CASE("State transitions for I/O blocking are logged", "[metrics][blocking]") {
  std::filesystem::create_directories("data/test/resultados");
  const std::string path = "data/test/resultados/test_io_blocking.jsonl";

  std::filesystem::remove(path);

  auto metrics = std::make_shared<MetricsCollector>();
  REQUIRE(metrics->enable_file_output(path));

  auto scheduler = std::make_unique<FCFSScheduler>();
  auto io_manager = std::make_shared<IOManager>();
  auto disk = std::make_shared<IODevice>("disk");
  disk->set_scheduler(std::make_unique<IOFCFSScheduler>());
  io_manager->add_device("disk", disk);
  
  CPUScheduler cpu_scheduler;
  cpu_scheduler.set_scheduler(std::move(scheduler));
  cpu_scheduler.set_io_manager(io_manager);
  cpu_scheduler.set_metrics_collector(metrics);

  std::vector<std::shared_ptr<Process>> processes;
  processes.push_back(std::make_shared<Process>(
      1, "P1", 0,
      std::vector<Burst>{Burst(BurstType::CPU, 2), Burst(BurstType::IO, 3, "disk"),
                         Burst(BurstType::CPU, 2)}));

  cpu_scheduler.load_processes(processes);

  for (int i = 0; i < 10; i++) {
    cpu_scheduler.execute_step(1);
  }

  metrics->flush_all();
  metrics->disable_output();

  REQUIRE(std::filesystem::exists(path));

  std::ifstream infile(path);
  std::string line;
  
  bool found_io_request = false;
  bool found_waiting_to_ready = false;

  while (std::getline(infile, line)) {
    if (line.empty())
      continue;
    auto j = json::parse(line);

    if (j.contains("state_transitions")) {
      for (const auto &st : j["state_transitions"]) {
        std::string reason = st["reason"];
        std::string to_state = st["to"];
        std::string from_state = st["from"];
        
        if (reason == "io_request" && to_state == "WAITING") {
          found_io_request = true;
        }
        if (from_state == "WAITING" && to_state == "READY") {
          found_waiting_to_ready = true;
        }
      }
    }
    
    // Also check old format for backward compatibility
    if (j.contains("state_transition")) {
      std::string reason = j["state_transition"]["reason"];
      std::string to_state = j["state_transition"]["to"];
      std::string from_state = j["state_transition"]["from"];
      
      if (reason == "io_request" && to_state == "WAITING") {
        found_io_request = true;
      }
      if (from_state == "WAITING" && to_state == "READY") {
        found_waiting_to_ready = true;
      }
    }
  }

  REQUIRE(found_io_request);
  REQUIRE(found_waiting_to_ready);
}

TEST_CASE("State transitions for memory blocking are logged", "[metrics][blocking]") {
  std::filesystem::create_directories("data/test/resultados");
  const std::string path = "data/test/resultados/test_memory_blocking.jsonl";

  std::filesystem::remove(path);

  auto metrics = std::make_shared<MetricsCollector>();
  REQUIRE(metrics->enable_file_output(path));

  auto scheduler = std::make_unique<FCFSScheduler>();
  auto memory_manager = std::make_shared<MemoryManager>(
      4, std::make_unique<FIFOReplacement>(), 1);
  
  CPUScheduler cpu_scheduler;
  cpu_scheduler.set_scheduler(std::move(scheduler));
  cpu_scheduler.set_memory_manager(memory_manager);
  cpu_scheduler.set_metrics_collector(metrics);

  std::vector<std::shared_ptr<Process>> processes;
  auto proc = std::make_shared<Process>(
      1, "P1", 0, std::vector<Burst>{Burst(BurstType::CPU, 5)});
  proc->memory_required = 4;  // 4 pages
  proc->memory_access_trace = {0, 1, 2, 3};  // Access all 4 pages
  processes.push_back(proc);

  cpu_scheduler.load_processes(processes);

  for (int i = 0; i < 15; i++) {
    cpu_scheduler.execute_step(1);
  }

  metrics->flush_all();
  metrics->disable_output();

  REQUIRE(std::filesystem::exists(path));

  std::ifstream infile(path);
  std::string line;
  
  bool found_page_fault = false;
  bool found_memory_waiting_to_ready = false;

  while (std::getline(infile, line)) {
    if (line.empty())
      continue;
    auto j = json::parse(line);

    if (j.contains("state_transitions")) {
      for (const auto &st : j["state_transitions"]) {
        std::string reason = st["reason"];
        std::string to_state = st["to"];
        std::string from_state = st["from"];
        
        if (reason == "page_fault" && to_state == "MEMORY_WAITING") {
          found_page_fault = true;
        }
        if (from_state == "MEMORY_WAITING" && to_state == "READY") {
          found_memory_waiting_to_ready = true;
        }
      }
    }
    
    // Also check old format
    if (j.contains("state_transition")) {
      std::string reason = j["state_transition"]["reason"];
      std::string to_state = j["state_transition"]["to"];
      std::string from_state = j["state_transition"]["from"];
      
      if (reason == "page_fault" && to_state == "MEMORY_WAITING") {
        found_page_fault = true;
      }
      if (from_state == "MEMORY_WAITING" && to_state == "READY") {
        found_memory_waiting_to_ready = true;
      }
    }
  }

  REQUIRE(found_page_fault);
  REQUIRE(found_memory_waiting_to_ready);
}

TEST_CASE("Queue snapshots are logged after blocking events", "[metrics][blocking]") {
  std::filesystem::create_directories("data/test/resultados");
  const std::string path = "data/test/resultados/test_blocking_queue_snapshots.jsonl";

  std::filesystem::remove(path);

  auto metrics = std::make_shared<MetricsCollector>();
  REQUIRE(metrics->enable_file_output(path));

  auto scheduler = std::make_unique<FCFSScheduler>();
  auto io_manager = std::make_shared<IOManager>();
  auto disk = std::make_shared<IODevice>("disk");
  disk->set_scheduler(std::make_unique<IOFCFSScheduler>());
  io_manager->add_device("disk", disk);
  
  CPUScheduler cpu_scheduler;
  cpu_scheduler.set_scheduler(std::move(scheduler));
  cpu_scheduler.set_io_manager(io_manager);
  cpu_scheduler.set_metrics_collector(metrics);

  std::vector<std::shared_ptr<Process>> processes;
  processes.push_back(std::make_shared<Process>(
      1, "P1", 0,
      std::vector<Burst>{Burst(BurstType::CPU, 1), Burst(BurstType::IO, 2, "disk")}));
  processes.push_back(
      std::make_shared<Process>(2, "P2", 0, std::vector<Burst>{Burst(BurstType::CPU, 3)}));

  cpu_scheduler.load_processes(processes);

  for (int i = 0; i < 8; i++) {
    cpu_scheduler.execute_step(1);
  }

  metrics->flush_all();
  metrics->disable_output();

  REQUIRE(std::filesystem::exists(path));

  std::ifstream infile(path);
  std::string line;
  
  bool found_blocking_snapshot = false;

  while (std::getline(infile, line)) {
    if (line.empty())
      continue;
    auto j = json::parse(line);

    if (j.contains("queues")) {
      auto blocked_io = j["queues"]["blocked_io"];
      if (blocked_io.is_array() && !blocked_io.empty()) {
        found_blocking_snapshot = true;
      }
    }
  }

  REQUIRE(found_blocking_snapshot);
}
