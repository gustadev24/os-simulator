/**
 * @file test_json_output.cpp
 * @brief Comprehensive tests for JSON output validation of IO and Memory modules.
 */

#include "core/burst.hpp"
#include "core/process.hpp"
#include "io/io_device.hpp"
#include "io/io_fcfs_scheduler.hpp"
#include "io/io_manager.hpp"
#include "memory/fifo_replacement.hpp"
#include "memory/memory_manager.hpp"
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <nlohmann/json.hpp>

using namespace OSSimulator;
using json = nlohmann::json;

TEST_CASE("IOManager JSON output validation", "[io][json]") {
  IOManager io_manager;

  // Setup devices
  auto disk = std::make_shared<IODevice>("disk");
  disk->set_scheduler(std::make_unique<IOFCFSScheduler>());
  io_manager.add_device("disk", disk);

  auto network = std::make_shared<IODevice>("network");
  network->set_scheduler(std::make_unique<IOFCFSScheduler>());
  io_manager.add_device("network", network);

  // Create processes and requests
  auto p1 = std::make_shared<Process>(1, "P1", 0, 10);
  auto p2 = std::make_shared<Process>(2, "P2", 0, 10);

  // Create bursts
  Burst disk_burst(BurstType::IO, 10, "disk");
  Burst net_burst(BurstType::IO, 5, "network");

  // Create requests
  auto req1 = std::make_shared<IORequest>(p1, disk_burst, 0);
  auto req2 = std::make_shared<IORequest>(p2, net_burst, 0);

  // Submit requests
  io_manager.submit_io_request(req1);
  io_manager.submit_io_request(req2);

  // Execute some steps
  io_manager.execute_all_devices(1, 0); // Step 1
  io_manager.execute_all_devices(1, 1); // Step 2

  SECTION("Validate JSON structure and content") {
    std::string json_str = io_manager.generate_json_output();
    json j = json::parse(json_str);

    // Check root object
    REQUIRE(j.contains("io_scheduler"));
    auto &scheduler = j["io_scheduler"];

    REQUIRE(scheduler.contains("total_devices"));
    REQUIRE(scheduler["total_devices"] == 2);

    REQUIRE(scheduler.contains("devices"));
    auto &devices = scheduler["devices"];
    REQUIRE(devices.is_array());
    REQUIRE(devices.size() == 2);

    // Find disk device
    bool disk_found = false;
    bool network_found = false;

    for (const auto &device : devices) {
      if (device["device_name"] == "disk") {
        disk_found = true;
        REQUIRE(device["algorithm"] == "FCFS");
        REQUIRE(device["is_busy"] == true);
        // Disk burst was 10, executed 2 ticks. Should still be busy.
      } else if (device["device_name"] == "network") {
        network_found = true;
        REQUIRE(device["algorithm"] == "FCFS");
        REQUIRE(device["is_busy"] == true);
      }
    }

    REQUIRE(disk_found);
    REQUIRE(network_found);
  }
}

TEST_CASE("MemoryManager JSON output validation", "[memory][json]") {
  auto replacement_algo = std::make_unique<FIFOReplacement>();
  // 4 frames, 1 tick latency
  MemoryManager memory_manager(4, std::move(replacement_algo), 1);

  // Create a test process
  auto process = std::make_shared<Process>();
  process->pid = 1;
  process->name = "TestProcess";
  process->memory_required = 3; // Requires 3 pages

  memory_manager.register_process(process);
  memory_manager.allocate_initial_memory(*process);

  // Trigger page faults by trying to prepare process for CPU
  memory_manager.prepare_process_for_cpu(process, 0);

  // Advance time to handle page faults
  // Process needs 3 pages. With latency 1, it should take 3 ticks to load all.
  memory_manager.advance_fault_queue(10, 0);

  SECTION("Validate JSON structure and content") {
    std::string json_str = memory_manager.generate_json_output();
    json j = json::parse(json_str);

    REQUIRE(j.contains("memory_manager"));
    auto &mm = j["memory_manager"];

    REQUIRE(mm.contains("total_frames"));
    REQUIRE(mm["total_frames"] == 4);

    REQUIRE(mm.contains("frames"));
    auto &frames = mm["frames"];
    REQUIRE(frames.is_array());
    REQUIRE(frames.size() == 4);

    // Check that some frames are occupied by PID 1
    int frames_occupied = 0;
    for (const auto &frame : frames) {
      if (frame.contains("process_id") && !frame["process_id"].is_null() &&
          frame["process_id"] == 1) {
        frames_occupied++;
      }
    }
    // Should have loaded 3 pages
    REQUIRE(frames_occupied == 3);

    REQUIRE(mm.contains("page_tables"));
    auto &page_tables = mm["page_tables"];
    REQUIRE(page_tables.is_array());

    bool pid_found = false;
    for (const auto &pt : page_tables) {
      if (pt.contains("pid") && pt["pid"] == 1) {
        pid_found = true;
        REQUIRE(pt.contains("pages"));
        REQUIRE(pt["pages"].is_array());
        REQUIRE(pt["pages"].size() == 3);
      }
    }
    REQUIRE(pid_found);

    REQUIRE(mm.contains("total_page_faults"));
    REQUIRE(mm["total_page_faults"] > 0);
  }
}
