#include "core/process.hpp"
#include "cpu/round_robin_scheduler.hpp"
#include <catch2/catch_test_macros.hpp>
#include <vector>

using namespace OSSimulator;

TEST_CASE("Round Robin Scheduler - Basic Operations", "[round_robin]") {
  RoundRobinScheduler scheduler(4);
  std::vector<Process> processes;

  SECTION("Empty scheduler") {
    REQUIRE_FALSE(scheduler.has_processes());
    REQUIRE(scheduler.size() == 0);
    REQUIRE(scheduler.get_next_process() == nullptr);
  }

  SECTION("Add single process") {
    processes.emplace_back(1, "P1", 0, 10);
    scheduler.add_process(&processes[0]);

    REQUIRE(scheduler.has_processes());
    REQUIRE(scheduler.size() == 1);
    REQUIRE(scheduler.get_next_process() != nullptr);
    REQUIRE(scheduler.get_next_process()->pid == 1);
  }

  SECTION("Algorithm type") {
    REQUIRE(scheduler.get_algorithm() == SchedulingAlgorithm::ROUND_ROBIN);
  }

  SECTION("Quantum configuration") {
    REQUIRE(scheduler.get_quantum() == 4);

    scheduler.set_quantum(8);
    REQUIRE(scheduler.get_quantum() == 8);
  }
}

TEST_CASE("Round Robin Scheduler - Queue Rotation", "[round_robin]") {
  RoundRobinScheduler scheduler(4);
  std::vector<Process> processes;

  SECTION("FIFO order before rotation") {
    processes.emplace_back(1, "P1", 0, 10);
    processes.emplace_back(2, "P2", 0, 5);
    processes.emplace_back(3, "P3", 0, 8);

    scheduler.add_process(&processes[0]);
    scheduler.add_process(&processes[1]);
    scheduler.add_process(&processes[2]);

    REQUIRE(scheduler.get_next_process()->pid == 1);
  }

  SECTION("Rotation moves front to back") {
    processes.clear();
    processes.emplace_back(1, "P1", 0, 10);
    processes.emplace_back(2, "P2", 0, 5);
    processes.emplace_back(3, "P3", 0, 8);

    scheduler.add_process(&processes[0]);
    scheduler.add_process(&processes[1]);
    scheduler.add_process(&processes[2]);

    REQUIRE(scheduler.get_next_process()->pid == 1);
    scheduler.rotate();
    REQUIRE(scheduler.get_next_process()->pid == 2);
    scheduler.rotate();
    REQUIRE(scheduler.get_next_process()->pid == 3);
    scheduler.rotate();
    REQUIRE(scheduler.get_next_process()->pid == 1);
  }

  SECTION("Multiple rotations") {
    processes.clear();
    processes.emplace_back(1, "P1", 0, 10);
    processes.emplace_back(2, "P2", 0, 5);

    scheduler.add_process(&processes[0]);
    scheduler.add_process(&processes[1]);

    for (int i = 0; i < 5; ++i) {
      scheduler.rotate();
    }

    REQUIRE(scheduler.get_next_process()->pid == 2);
  }
}

TEST_CASE("Round Robin Scheduler - Remove Operations", "[round_robin]") {
  RoundRobinScheduler scheduler(4);
  std::vector<Process> processes;

  SECTION("Remove from queue") {
    processes.emplace_back(1, "P1", 0, 10);
    processes.emplace_back(2, "P2", 0, 5);
    processes.emplace_back(3, "P3", 0, 8);

    scheduler.add_process(&processes[0]);
    scheduler.add_process(&processes[1]);
    scheduler.add_process(&processes[2]);

    scheduler.remove_process(2);
    REQUIRE(scheduler.size() == 2);
    REQUIRE(scheduler.get_next_process()->pid == 1);
  }

  SECTION("Remove and rotate") {
    processes.clear();
    processes.emplace_back(1, "P1", 0, 10);
    processes.emplace_back(2, "P2", 0, 5);
    processes.emplace_back(3, "P3", 0, 8);

    scheduler.add_process(&processes[0]);
    scheduler.add_process(&processes[1]);
    scheduler.add_process(&processes[2]);

    scheduler.rotate();
    scheduler.remove_process(1);

    REQUIRE(scheduler.size() == 2);
    REQUIRE(scheduler.get_next_process()->pid == 2);
  }

  SECTION("Clear scheduler") {
    processes.clear();
    processes.emplace_back(1, "P1", 0, 10);
    processes.emplace_back(2, "P2", 0, 5);

    scheduler.add_process(&processes[0]);
    scheduler.add_process(&processes[1]);

    scheduler.clear();
    REQUIRE(scheduler.size() == 0);
    REQUIRE_FALSE(scheduler.has_processes());
  }
}

TEST_CASE("Round Robin Scheduler - Different Quantum Values", "[round_robin]") {
  SECTION("Quantum 1") {
    RoundRobinScheduler scheduler(1);
    REQUIRE(scheduler.get_quantum() == 1);
  }

  SECTION("Quantum 10") {
    RoundRobinScheduler scheduler(10);
    REQUIRE(scheduler.get_quantum() == 10);
  }

  SECTION("Default quantum") {
    RoundRobinScheduler scheduler;
    REQUIRE(scheduler.get_quantum() == 4);
  }
}
