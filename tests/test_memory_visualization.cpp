#include "cpu/cpu_scheduler.hpp"
#include "cpu/fcfs_scheduler.hpp"
#include "memory/fifo_replacement.hpp"
#include "memory/memory_manager.hpp"
#include "metrics/metrics_collector.hpp"
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <set>
#include <nlohmann/json.hpp>

using namespace OSSimulator;
using json = nlohmann::json;

TEST_CASE("Page table snapshots are logged", "[metrics][visualization]") {
  std::filesystem::create_directories("data/test/resultados");
  const std::string path = "data/test/resultados/test_page_table_snapshot.jsonl";

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

  // Process with 2 pages
  std::vector<std::shared_ptr<Process>> processes;
  auto proc = std::make_shared<Process>(
      1, "P1", 0, std::vector<Burst>{Burst(BurstType::CPU, 5)});
  proc->memory_required = 2;  // 2 pages
  proc->memory_access_trace = {0, 1};
  processes.push_back(proc);

  cpu_scheduler.load_processes(processes);

  // Run simulation
  for (int i = 0; i < 10; i++) {
    cpu_scheduler.execute_step(1);
  }

  metrics->flush_all();
  metrics->disable_output();

  REQUIRE(std::filesystem::exists(path));

  std::ifstream infile(path);
  REQUIRE(infile.is_open());

  std::string line;
  bool found_page_table = false;
  bool found_frame_status = false;

  while (std::getline(infile, line)) {
    if (line.empty())
      continue;
    auto j = json::parse(line);

    if (j.contains("page_table")) {
      found_page_table = true;
      REQUIRE(j["page_table"]["pid"] == 1);
      REQUIRE(j["page_table"]["name"] == "P1");
      REQUIRE(j["page_table"]["pages"].is_array());
      
      // Check that page table entries have the right fields
      if (!j["page_table"]["pages"].empty()) {
        auto &page = j["page_table"]["pages"][0];
        REQUIRE(page.contains("page"));
        REQUIRE(page.contains("frame"));
        REQUIRE(page.contains("valid"));
        REQUIRE(page.contains("referenced"));
        REQUIRE(page.contains("modified"));
      }
    }

    if (j.contains("frame_status")) {
      found_frame_status = true;
      REQUIRE(j["frame_status"].is_array());
      
      // Should have 4 frames (as configured)
      REQUIRE(j["frame_status"].size() == 4);
      
      // Check that frame entries have the right fields
      auto &frame = j["frame_status"][0];
      REQUIRE(frame.contains("frame"));
      REQUIRE(frame.contains("occupied"));
      REQUIRE(frame.contains("pid"));
      REQUIRE(frame.contains("page"));
    }
  }

  REQUIRE(found_page_table);
  REQUIRE(found_frame_status);
}

TEST_CASE("Frame status shows memory allocation", "[metrics][visualization]") {
  std::filesystem::create_directories("data/test/resultados");
  const std::string path = "data/test/resultados/test_frame_allocation.jsonl";

  std::filesystem::remove(path);

  auto metrics = std::make_shared<MetricsCollector>();
  REQUIRE(metrics->enable_file_output(path));

  auto scheduler = std::make_unique<FCFSScheduler>();
  auto memory_manager = std::make_shared<MemoryManager>(
      3, std::make_unique<FIFOReplacement>(), 1);
  
  CPUScheduler cpu_scheduler;
  cpu_scheduler.set_scheduler(std::move(scheduler));
  cpu_scheduler.set_memory_manager(memory_manager);
  cpu_scheduler.set_metrics_collector(metrics);

  // Process with pages
  std::vector<std::shared_ptr<Process>> processes;
  auto proc1 = std::make_shared<Process>(
      1, "P1", 0, std::vector<Burst>{Burst(BurstType::CPU, 3)});
  proc1->memory_required = 2;
  proc1->memory_access_trace = {0, 1};
  processes.push_back(proc1);

  cpu_scheduler.load_processes(processes);

  // Run simulation
  for (int i = 0; i < 10; i++) {
    cpu_scheduler.execute_step(1);
  }

  metrics->flush_all();
  metrics->disable_output();

  REQUIRE(std::filesystem::exists(path));

  std::ifstream infile(path);
  std::string line;
  
  bool found_occupied_frame = false;
  
  while (std::getline(infile, line)) {
    if (line.empty())
      continue;
    auto j = json::parse(line);

    if (j.contains("frame_status")) {
      for (const auto &frame : j["frame_status"]) {
        if (frame["occupied"] && frame["pid"] == 1) {
          found_occupied_frame = true;
          // Verify the frame has valid data
          REQUIRE(frame["frame"] >= 0);
          REQUIRE(frame["page"] >= 0);
        }
      }
    }
  }

  // Verify we logged at least one occupied frame for the process
  REQUIRE(found_occupied_frame);
}
