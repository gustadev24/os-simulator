#include "core/process.hpp"
#include "cpu/fcfs_scheduler.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace OSSimulator;

TEST_CASE("FCFS Scheduler - Basic Operations", "[fcfs]") {
  FCFSScheduler scheduler;

  SECTION("Empty scheduler") {
    REQUIRE_FALSE(scheduler.has_processes());
    REQUIRE(scheduler.size() == 0);
    REQUIRE(scheduler.get_next_process() == nullptr);
  }

  SECTION("Add single process") {
    Process p1(1, "P1", 0, 10);
    scheduler.add_process(p1);

    REQUIRE(scheduler.has_processes());
    REQUIRE(scheduler.size() == 1);
    REQUIRE(scheduler.get_next_process() != nullptr);
    REQUIRE(scheduler.get_next_process()->pid == 1);
  }

  SECTION("Add multiple processes") {
    Process p1(1, "P1", 0, 10);
    Process p2(2, "P2", 1, 5);
    Process p3(3, "P3", 2, 8);

    scheduler.add_process(p1);
    scheduler.add_process(p2);
    scheduler.add_process(p3);

    REQUIRE(scheduler.size() == 3);
    REQUIRE(scheduler.get_next_process()->pid == 1);
  }

  SECTION("FCFS order - first come first served") {
    Process p1(1, "P1", 0, 10);
    Process p2(2, "P2", 1, 5);
    Process p3(3, "P3", 2, 8);

    scheduler.add_process(p1);
    scheduler.add_process(p2);
    scheduler.add_process(p3);

    REQUIRE(scheduler.get_next_process()->pid == 1);
    scheduler.remove_process(1);

    REQUIRE(scheduler.get_next_process()->pid == 2);
    scheduler.remove_process(2);

    REQUIRE(scheduler.get_next_process()->pid == 3);
  }

  SECTION("Remove process") {
    Process p1(1, "P1", 0, 10);
    Process p2(2, "P2", 1, 5);

    scheduler.add_process(p1);
    scheduler.add_process(p2);

    scheduler.remove_process(1);
    REQUIRE(scheduler.size() == 1);
    REQUIRE(scheduler.get_next_process()->pid == 2);
  }

  SECTION("Clear scheduler") {
    Process p1(1, "P1", 0, 10);
    Process p2(2, "P2", 1, 5);

    scheduler.add_process(p1);
    scheduler.add_process(p2);

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

  SECTION("Different arrival times") {
    Process p1(1, "P1", 5, 10);
    Process p2(2, "P2", 0, 5);
    Process p3(3, "P3", 3, 8);

    scheduler.add_process(p1);
    scheduler.add_process(p2);
    scheduler.add_process(p3);

    REQUIRE(scheduler.get_next_process()->pid == 1);
    scheduler.remove_process(1);
    REQUIRE(scheduler.get_next_process()->pid == 2);
    scheduler.remove_process(2);
    REQUIRE(scheduler.get_next_process()->pid == 3);
  }
}
