#include "core/process.hpp"
#include "cpu/priority_scheduler.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace OSSimulator;

TEST_CASE("Priority Scheduler - Basic Operations", "[priority]") {
  PriorityScheduler scheduler;

  SECTION("Empty scheduler") {
    REQUIRE_FALSE(scheduler.has_processes());
    REQUIRE(scheduler.size() == 0);
    REQUIRE(scheduler.get_next_process() == nullptr);
  }

  SECTION("Add single process") {
    Process p1(1, "P1", 0, 10, 5);
    scheduler.add_process(p1);

    REQUIRE(scheduler.has_processes());
    REQUIRE(scheduler.size() == 1);
    REQUIRE(scheduler.get_next_process() != nullptr);
    REQUIRE(scheduler.get_next_process()->pid == 1);
  }

  SECTION("Algorithm type") {
    REQUIRE(scheduler.get_algorithm() == SchedulingAlgorithm::PRIORITY);
  }
}

TEST_CASE("Priority Scheduler - Priority Order", "[priority]") {
  PriorityScheduler scheduler;

  SECTION("Lower priority number = higher priority") {
    Process p1(1, "P1", 0, 10, 5);
    Process p2(2, "P2", 0, 5, 1);
    Process p3(3, "P3", 0, 8, 3);

    scheduler.add_process(p1);
    scheduler.add_process(p2);
    scheduler.add_process(p3);

    REQUIRE(scheduler.get_next_process()->pid == 2);
    scheduler.remove_process(2);

    REQUIRE(scheduler.get_next_process()->pid == 3);
    scheduler.remove_process(3);

    REQUIRE(scheduler.get_next_process()->pid == 1);
  }

  SECTION("Equal priority - arrival time breaks tie") {
    Process p1(1, "P1", 2, 10, 3);
    Process p2(2, "P2", 0, 5, 3);
    Process p3(3, "P3", 1, 8, 3);

    scheduler.add_process(p1);
    scheduler.add_process(p2);
    scheduler.add_process(p3);

    REQUIRE(scheduler.get_next_process()->pid == 2);
    scheduler.remove_process(2);

    REQUIRE(scheduler.get_next_process()->pid == 3);
    scheduler.remove_process(3);

    REQUIRE(scheduler.get_next_process()->pid == 1);
  }

  SECTION("Mixed priorities") {
    Process p1(1, "P1", 0, 10, 0);
    Process p2(2, "P2", 0, 5, 10);
    Process p3(3, "P3", 0, 8, 5);
    Process p4(4, "P4", 0, 3, 2);

    scheduler.add_process(p1);
    scheduler.add_process(p2);
    scheduler.add_process(p3);
    scheduler.add_process(p4);

    REQUIRE(scheduler.get_next_process()->pid == 1);
    scheduler.remove_process(1);

    REQUIRE(scheduler.get_next_process()->pid == 4);
    scheduler.remove_process(4);

    REQUIRE(scheduler.get_next_process()->pid == 3);
    scheduler.remove_process(3);

    REQUIRE(scheduler.get_next_process()->pid == 2);
  }
}

TEST_CASE("Priority Scheduler - Remove and Clear", "[priority]") {
  PriorityScheduler scheduler;

  SECTION("Remove process maintains order") {
    Process p1(1, "P1", 0, 10, 5);
    Process p2(2, "P2", 0, 5, 1);
    Process p3(3, "P3", 0, 8, 3);

    scheduler.add_process(p1);
    scheduler.add_process(p2);
    scheduler.add_process(p3);

    scheduler.remove_process(2);

    REQUIRE(scheduler.size() == 2);
    REQUIRE(scheduler.get_next_process()->pid == 3);
  }

  SECTION("Clear scheduler") {
    Process p1(1, "P1", 0, 10, 5);
    Process p2(2, "P2", 0, 5, 1);

    scheduler.add_process(p1);
    scheduler.add_process(p2);

    scheduler.clear();
    REQUIRE(scheduler.size() == 0);
    REQUIRE_FALSE(scheduler.has_processes());
  }
}

TEST_CASE("Priority Scheduler - Edge Cases", "[priority]") {
  PriorityScheduler scheduler;

  SECTION("All same priority and arrival time") {
    Process p1(1, "P1", 0, 10, 5);
    Process p2(2, "P2", 0, 5, 5);
    Process p3(3, "P3", 0, 8, 5);

    scheduler.add_process(p1);
    scheduler.add_process(p2);
    scheduler.add_process(p3);

    REQUIRE(scheduler.get_next_process()->pid == 1);
    scheduler.remove_process(1);

    REQUIRE(scheduler.get_next_process()->pid == 2);
    scheduler.remove_process(2);

    REQUIRE(scheduler.get_next_process()->pid == 3);
  }

  SECTION("Priority 0 is highest") {
    Process p1(1, "P1", 0, 10, 0);
    Process p2(2, "P2", 0, 5, 1);

    scheduler.add_process(p2);
    scheduler.add_process(p1);

    REQUIRE(scheduler.get_next_process()->pid == 1);
  }
}
