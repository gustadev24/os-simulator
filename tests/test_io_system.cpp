/**
 * @file test_io_system.cpp
 * @brief Tests para el sistema completo de E/S (bursts, requests, devices, managers)
 */

#include "core/burst.hpp"
#include "core/process.hpp"
#include "io/io_device.hpp"
#include "io/io_fcfs_scheduler.hpp"
#include "io/io_manager.hpp"
#include "io/io_request.hpp"
#include "io/io_round_robin_scheduler.hpp"
#include <catch2/catch_test_macros.hpp>
#include <memory>

using namespace OSSimulator;

// ============================================================================
// BURST TESTS
// ============================================================================

TEST_CASE("Burst creation and management", "[io][burst]") {
  SECTION("CPU burst") {
    Burst burst(BurstType::CPU, 10);
    REQUIRE(burst.type == BurstType::CPU);
    REQUIRE(burst.duration == 10);
    REQUIRE(burst.remaining_time == 10);
    REQUIRE_FALSE(burst.is_completed());
  }

  SECTION("IO burst with device") {
    Burst burst(BurstType::IO, 5, "disk");
    REQUIRE(burst.type == BurstType::IO);
    REQUIRE(burst.duration == 5);
    REQUIRE(burst.remaining_time == 5);
    REQUIRE(burst.io_device == "disk");
  }

  SECTION("Burst reset") {
    Burst burst(BurstType::CPU, 10);
    burst.remaining_time = 5;
    burst.reset();
    REQUIRE(burst.remaining_time == 10);
  }
}

// ============================================================================
// PROCESS WITH BURSTS TESTS
// ============================================================================

TEST_CASE("Process with burst sequences", "[io][process]") {
  SECTION("Single CPU burst process") {
    auto proc = std::make_shared<Process>(1, "P1", 0, 10);
    REQUIRE(proc->burst_sequence.size() == 1);
    REQUIRE(proc->burst_sequence[0].type == BurstType::CPU);
    REQUIRE(proc->total_cpu_time == 10);
    REQUIRE(proc->total_io_time == 0);
  }

  SECTION("Process with CPU and IO bursts") {
    std::vector<Burst> bursts = {Burst(BurstType::CPU, 4),
                                 Burst(BurstType::IO, 3, "disk"),
                                 Burst(BurstType::CPU, 5)};

    auto proc = std::make_shared<Process>(1, "P1", 0, bursts);

    REQUIRE(proc->burst_sequence.size() == 3);
    REQUIRE(proc->total_cpu_time == 9);
    REQUIRE(proc->total_io_time == 3);
    REQUIRE(proc->current_burst_index == 0);
  }

  SECTION("Burst navigation") {
    std::vector<Burst> bursts = {Burst(BurstType::CPU, 4),
                                 Burst(BurstType::IO, 3, "disk"),
                                 Burst(BurstType::CPU, 5)};

    auto proc = std::make_shared<Process>(1, "P1", 0, bursts);

    REQUIRE(proc->is_on_cpu_burst());
    REQUIRE_FALSE(proc->is_on_io_burst());

    proc->current_burst_index = 1;
    REQUIRE_FALSE(proc->is_on_cpu_burst());
    REQUIRE(proc->is_on_io_burst());

    proc->current_burst_index = 2;
    REQUIRE(proc->is_on_cpu_burst());
    REQUIRE_FALSE(proc->is_on_io_burst());
  }
}

// ============================================================================
// IO REQUEST TESTS
// ============================================================================

TEST_CASE("IO Request management", "[io][request]") {
  auto proc = std::make_shared<Process>(1, "P1", 0, 10);
  Burst io_burst(BurstType::IO, 5, "disk");

  IORequest request(proc, io_burst, 10, 1);

  REQUIRE(request.process == proc);
  REQUIRE(request.burst.duration == 5);
  REQUIRE(request.arrival_time == 10);
  REQUIRE(request.priority == 1);
  REQUIRE(request.start_time == -1);
  REQUIRE_FALSE(request.is_completed());

  SECTION("Execute IO request") {
    int time_executed = request.execute(0, 10);
    REQUIRE(time_executed == 5);
    REQUIRE(request.is_completed());
    REQUIRE(request.start_time == 10);
    REQUIRE(request.completion_time == 15);
  }

  SECTION("Execute with quantum") {
    int time1 = request.execute(2, 10);
    REQUIRE(time1 == 2);
    REQUIRE(request.burst.remaining_time == 3);
    REQUIRE_FALSE(request.is_completed());

    int time2 = request.execute(2, 12);
    REQUIRE(time2 == 2);
    REQUIRE(request.burst.remaining_time == 1);

    int time3 = request.execute(2, 14);
    REQUIRE(time3 == 1);
    REQUIRE(request.is_completed());
    REQUIRE(request.completion_time == 15);
  }
}

// ============================================================================
// IO SCHEDULER TESTS
// ============================================================================

TEST_CASE("IO FCFS Scheduler", "[io][scheduler][fcfs]") {
  IOFCFSScheduler scheduler;

  auto proc1 = std::make_shared<Process>(1, "P1", 0, 10);
  auto proc2 = std::make_shared<Process>(2, "P2", 1, 8);
  auto proc3 = std::make_shared<Process>(3, "P3", 2, 6);

  auto req1 =
      std::make_shared<IORequest>(proc1, Burst(BurstType::IO, 5, "disk"), 10);
  auto req2 =
      std::make_shared<IORequest>(proc2, Burst(BurstType::IO, 3, "disk"), 11);
  auto req3 =
      std::make_shared<IORequest>(proc3, Burst(BurstType::IO, 4, "disk"), 12);

  SECTION("FCFS order") {
    scheduler.add_request(req1);
    scheduler.add_request(req2);
    scheduler.add_request(req3);

    REQUIRE(scheduler.size() == 3);
    REQUIRE(scheduler.has_requests());

    auto next1 = scheduler.get_next_request();
    REQUIRE(next1 == req1);

    auto next2 = scheduler.get_next_request();
    REQUIRE(next2 == req2);

    auto next3 = scheduler.get_next_request();
    REQUIRE(next3 == req3);

    REQUIRE_FALSE(scheduler.has_requests());
  }
}

TEST_CASE("IO Round Robin Scheduler", "[io][scheduler][rr]") {
  IORoundRobinScheduler scheduler(4);

  REQUIRE(scheduler.get_quantum() == 4);
  REQUIRE(scheduler.get_algorithm() == IOSchedulingAlgorithm::ROUND_ROBIN);

  auto proc1 = std::make_shared<Process>(1, "P1", 0, 10);
  auto proc2 = std::make_shared<Process>(2, "P2", 1, 8);

  auto req1 =
      std::make_shared<IORequest>(proc1, Burst(BurstType::IO, 10, "disk"), 0);
  auto req2 =
      std::make_shared<IORequest>(proc2, Burst(BurstType::IO, 8, "disk"), 1);

  scheduler.add_request(req1);
  scheduler.add_request(req2);

  REQUIRE(scheduler.size() == 2);

  auto next1 = scheduler.get_next_request();
  REQUIRE(next1 == req1);

  auto next2 = scheduler.get_next_request();
  REQUIRE(next2 == req2);

  REQUIRE(scheduler.size() == 0);
}

// ============================================================================
// IO DEVICE TESTS
// ============================================================================

TEST_CASE("IO Device with FCFS scheduling", "[io][device]") {
  IODevice device("disk");
  device.set_scheduler(std::make_unique<IOFCFSScheduler>());

  int completed_count = 0;
  int last_completion_time = 0;

  device.set_completion_callback(
      [&](std::shared_ptr<Process> /*proc*/, int time) {
        completed_count++;
        last_completion_time = time;
      });

  auto proc1 = std::make_shared<Process>(1, "P1", 0, 10);
  auto req1 =
      std::make_shared<IORequest>(proc1, Burst(BurstType::IO, 5, "disk"), 0);

  device.add_io_request(req1);
  REQUIRE(device.has_pending_requests());

  device.execute_step(0, 0);

  REQUIRE(completed_count == 1);
  REQUIRE(last_completion_time == 5);
  REQUIRE_FALSE(device.has_pending_requests());
}

TEST_CASE("IO Device with Round Robin scheduling", "[io][device][rr]") {
  IODevice device("disk");
  device.set_scheduler(std::make_unique<IORoundRobinScheduler>(4));

  int completed_count = 0;
  device.set_completion_callback([&](std::shared_ptr<Process> /*proc*/,
                                     int /*time*/) { completed_count++; });

  auto proc1 = std::make_shared<Process>(1, "P1", 0, 10);
  auto proc2 = std::make_shared<Process>(2, "P2", 1, 8);

  auto req1 =
      std::make_shared<IORequest>(proc1, Burst(BurstType::IO, 10, "disk"), 0);
  auto req2 =
      std::make_shared<IORequest>(proc2, Burst(BurstType::IO, 6, "disk"), 1);

  device.add_io_request(req1);
  device.add_io_request(req2);

  int current_time = 0;

  device.execute_step(4, current_time);
  current_time += 4;
  REQUIRE(device.has_pending_requests());
  REQUIRE(completed_count == 0);

  device.execute_step(4, current_time);
  current_time += 4;
  REQUIRE(device.has_pending_requests());
  REQUIRE(completed_count == 0);

  device.execute_step(4, current_time);
  current_time += 4;
  REQUIRE(device.has_pending_requests());
  REQUIRE(completed_count == 0);

  device.execute_step(4, current_time);
  current_time += 2;
  REQUIRE(device.has_pending_requests());
  REQUIRE(completed_count == 1);

  device.execute_step(4, current_time);
  current_time += 2;
  REQUIRE_FALSE(device.has_pending_requests());
  REQUIRE(completed_count == 2);
}

// ============================================================================
// IO MANAGER TESTS
// ============================================================================

TEST_CASE("IO Manager with multiple devices", "[io][manager]") {
  IOManager manager;

  auto disk_device = std::make_shared<IODevice>("disk");
  disk_device->set_scheduler(std::make_unique<IOFCFSScheduler>());

  auto tape_device = std::make_shared<IODevice>("tape");
  tape_device->set_scheduler(std::make_unique<IOFCFSScheduler>());

  manager.add_device("disk", disk_device);
  manager.add_device("tape", tape_device);

  REQUIRE(manager.has_device("disk"));
  REQUIRE(manager.has_device("tape"));
  REQUIRE_FALSE(manager.has_device("network"));

  int completed_count = 0;
  manager.set_completion_callback([&](std::shared_ptr<Process> /*proc*/,
                                      int /*time*/) { completed_count++; });

  auto proc1 = std::make_shared<Process>(1, "P1", 0, 10);
  auto req1 =
      std::make_shared<IORequest>(proc1, Burst(BurstType::IO, 5, "disk"), 0);

  manager.submit_io_request(req1);

  REQUIRE(manager.has_pending_io());

  manager.execute_all_devices(0, 0);

  REQUIRE(completed_count == 1);
  REQUIRE_FALSE(manager.has_pending_io());
}
