#include "core/process.hpp"
#include "cpu/sjf_scheduler.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace OSSimulator;

TEST_CASE("SJF Scheduler - Basic Operations", "[sjf]") {
  SJFScheduler scheduler;

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

  SECTION("Algorithm type") {
    REQUIRE(scheduler.get_algorithm() == SchedulingAlgorithm::SJF);
  }
}

TEST_CASE("SJF Scheduler - Shortest Job First Order", "[sjf]") {
  SJFScheduler scheduler;

  SECTION("Processes sorted by burst time") {
    Process p1(1, "P1", 0, 10);
    Process p2(2, "P2", 0, 5);
    Process p3(3, "P3", 0, 8);

    scheduler.add_process(p1);
    scheduler.add_process(p2);
    scheduler.add_process(p3);

    REQUIRE(scheduler.get_next_process()->pid == 2);
    scheduler.remove_process(2);

    REQUIRE(scheduler.get_next_process()->pid == 3);
    scheduler.remove_process(3);

    REQUIRE(scheduler.get_next_process()->pid == 1);
  }

  SECTION("Equal burst time - arrival time breaks tie") {
    Process p1(1, "P1", 2, 5);
    Process p2(2, "P2", 0, 5);
    Process p3(3, "P3", 1, 5);

    scheduler.add_process(p1);
    scheduler.add_process(p2);
    scheduler.add_process(p3);

    REQUIRE(scheduler.get_next_process()->pid == 2);
    scheduler.remove_process(2);

    REQUIRE(scheduler.get_next_process()->pid == 3);
    scheduler.remove_process(3);

    REQUIRE(scheduler.get_next_process()->pid == 1);
  }

  SECTION("Dynamic sorting with remaining time") {
    Process p1(1, "P1", 0, 10);
    p1.remaining_time = 3;

    Process p2(2, "P2", 0, 5);
    p2.remaining_time = 5;

    Process p3(3, "P3", 0, 8);
    p3.remaining_time = 2;

    scheduler.add_process(p1);
    scheduler.add_process(p2);
    scheduler.add_process(p3);

    REQUIRE(scheduler.get_next_process()->pid == 3);
    scheduler.remove_process(3);

    REQUIRE(scheduler.get_next_process()->pid == 1);
    scheduler.remove_process(1);

    REQUIRE(scheduler.get_next_process()->pid == 2);
  }
}

TEST_CASE("SJF Scheduler - Remove and Clear", "[sjf]") {
  SJFScheduler scheduler;

  SECTION("Remove process maintains order") {
    Process p1(1, "P1", 0, 10);
    Process p2(2, "P2", 0, 5);
    Process p3(3, "P3", 0, 8);

    scheduler.add_process(p1);
    scheduler.add_process(p2);
    scheduler.add_process(p3);

    scheduler.remove_process(2);

    REQUIRE(scheduler.size() == 2);
    REQUIRE(scheduler.get_next_process()->pid == 3);
  }

  SECTION("Clear scheduler") {
    Process p1(1, "P1", 0, 10);
    Process p2(2, "P2", 0, 5);

    scheduler.add_process(p1);
    scheduler.add_process(p2);

    scheduler.clear();
    REQUIRE(scheduler.size() == 0);
    REQUIRE_FALSE(scheduler.has_processes());
  }
}
