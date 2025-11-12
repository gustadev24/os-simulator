#include "core/process.hpp"
#include "cpu/fcfs_scheduler.hpp"
#include <catch2/catch_test_macros.hpp>
#include <vector>

using namespace OSSimulator;

TEST_CASE("FCFS Scheduler - Basic Operations", "[fcfs]") {
  FCFSScheduler scheduler;
  std::vector<std::shared_ptr<Process>> processes; // Store processes to keep them alive

  SECTION("Empty scheduler") {
    REQUIRE_FALSE(scheduler.has_processes());
    REQUIRE(scheduler.size() == 0);
    REQUIRE(scheduler.get_next_process() == nullptr);
  }

  SECTION("Add single process") {
    processes.push_back(std::make_shared<Process>(1, "P1", 0, 10));
    scheduler.add_process(processes[0]);

    REQUIRE(scheduler.has_processes());
    REQUIRE(scheduler.size() == 1);
    REQUIRE(scheduler.get_next_process() != nullptr);
    REQUIRE(scheduler.get_next_process()->pid == 1);
  }

  SECTION("Add multiple processes") {
    processes.push_back(std::make_shared<Process>(1, "P1", 0, 10));
    processes.push_back(std::make_shared<Process>(2, "P2", 1, 5));
    processes.push_back(std::make_shared<Process>(3, "P3", 2, 8));

    scheduler.add_process(processes[0]);
    scheduler.add_process(processes[1]);
    scheduler.add_process(processes[2]);

    REQUIRE(scheduler.size() == 3);
    REQUIRE(scheduler.get_next_process()->pid == 1);
  }

  SECTION("FCFS order - first come first served") {
    processes.push_back(std::make_shared<Process>(1, "P1", 0, 10));
    processes.push_back(std::make_shared<Process>(2, "P2", 1, 5));
    processes.push_back(std::make_shared<Process>(3, "P3", 2, 8));

    scheduler.add_process(processes[0]);
    scheduler.add_process(processes[1]);
    scheduler.add_process(processes[2]);

    REQUIRE(scheduler.get_next_process()->pid == 1);
    scheduler.remove_process(1);

    REQUIRE(scheduler.get_next_process()->pid == 2);
    scheduler.remove_process(2);

    REQUIRE(scheduler.get_next_process()->pid == 3);
  }

  SECTION("Remove process") {
    processes.push_back(std::make_shared<Process>(1, "P1", 0, 10));
    processes.push_back(std::make_shared<Process>(2, "P2", 1, 5));

    scheduler.add_process(processes[0]);
    scheduler.add_process(processes[1]);

    scheduler.remove_process(1);
    REQUIRE(scheduler.size() == 1);
    REQUIRE(scheduler.get_next_process()->pid == 2);
  }

  SECTION("Clear scheduler") {
    processes.push_back(std::make_shared<Process>(1, "P1", 0, 10));
    processes.push_back(std::make_shared<Process>(2, "P2", 1, 5));

    scheduler.add_process(processes[0]);
    scheduler.add_process(processes[1]);

    scheduler.clear();
    REQUIRE(scheduler.size() == 0);
    REQUIRE_FALSE(scheduler.has_processes());
  }

  SECTION("Algorithm type") {
    REQUIRE(scheduler.get_algorithm() == SchedulingAlgorithm::FCFS);
  }
}

TEST_CASE("FCFS Scheduler - Arrival Order", "[fcfs]") {
  FCFSScheduler scheduler;
  std::vector<std::shared_ptr<Process>> processes;

  SECTION("Different arrival times") {
    processes.push_back(std::make_shared<Process>(1, "P1", 5, 10));
    processes.push_back(std::make_shared<Process>(2, "P2", 0, 5));
    processes.push_back(std::make_shared<Process>(3, "P3", 3, 8));

    scheduler.add_process(processes[0]);
    scheduler.add_process(processes[1]);
    scheduler.add_process(processes[2]);

    REQUIRE(scheduler.get_next_process()->pid == 1);
    scheduler.remove_process(1);
    REQUIRE(scheduler.get_next_process()->pid == 2);
    scheduler.remove_process(2);
    REQUIRE(scheduler.get_next_process()->pid == 3);
  }
}
