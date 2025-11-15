#include "core/process.hpp"
#include "cpu/sjf_scheduler.hpp"
#include <catch2/catch_test_macros.hpp>
#include <vector>

using namespace OSSimulator;

TEST_CASE("SJF Scheduler - Basic Operations", "[sjf]") {
  SJFScheduler scheduler;
  std::vector<std::shared_ptr<Process>> processes;

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

  SECTION("Algorithm type") {
    REQUIRE(scheduler.get_algorithm() == SchedulingAlgorithm::SJF);
  }
}

TEST_CASE("SJF Scheduler - Shortest Job First Order", "[sjf]") {
  SJFScheduler scheduler;
  std::vector<std::shared_ptr<Process>> processes;

  SECTION("Processes sorted by burst time") {
    processes.push_back(std::make_shared<Process>(1, "P1", 0, 10));
    processes.push_back(std::make_shared<Process>(2, "P2", 0, 5));
    processes.push_back(std::make_shared<Process>(3, "P3", 0, 8));

    scheduler.add_process(processes[0]);
    scheduler.add_process(processes[1]);
    scheduler.add_process(processes[2]);

    REQUIRE(scheduler.get_next_process()->pid == 2);
    scheduler.remove_process(2);

    REQUIRE(scheduler.get_next_process()->pid == 3);
    scheduler.remove_process(3);

    REQUIRE(scheduler.get_next_process()->pid == 1);
  }

  SECTION("Equal burst time - arrival time breaks tie") {
    processes.clear();
    processes.push_back(std::make_shared<Process>(1, "P1", 2, 5));
    processes.push_back(std::make_shared<Process>(2, "P2", 0, 5));
    processes.push_back(std::make_shared<Process>(3, "P3", 1, 5));

    scheduler.add_process(processes[0]);
    scheduler.add_process(processes[1]);
    scheduler.add_process(processes[2]);

    REQUIRE(scheduler.get_next_process()->pid == 2);
    scheduler.remove_process(2);

    REQUIRE(scheduler.get_next_process()->pid == 3);
    scheduler.remove_process(3);

    REQUIRE(scheduler.get_next_process()->pid == 1);
  }

  SECTION("Dynamic sorting with remaining time") {
    processes.clear();
    auto p1 = std::make_shared<Process>(1, "P1", 0, 10);
    p1->remaining_time = 3;
    processes.push_back(p1);

    auto p2 = std::make_shared<Process>(2, "P2", 0, 5);
    p2->remaining_time = 5;
    processes.push_back(p2);

    auto p3 = std::make_shared<Process>(3, "P3", 0, 8);
    p3->remaining_time = 2;
    processes.push_back(p3);

    scheduler.add_process(processes[0]);
    scheduler.add_process(processes[1]);
    scheduler.add_process(processes[2]);

    REQUIRE(scheduler.get_next_process()->pid == 3);
    scheduler.remove_process(3);

    REQUIRE(scheduler.get_next_process()->pid == 1);
    scheduler.remove_process(1);

    REQUIRE(scheduler.get_next_process()->pid == 2);
  }
}

TEST_CASE("SJF Scheduler - Remove and Clear", "[sjf]") {
  SJFScheduler scheduler;
  std::vector<std::shared_ptr<Process>> processes;

  SECTION("Remove process maintains order") {
    processes.push_back(std::make_shared<Process>(1, "P1", 0, 10));
    processes.push_back(std::make_shared<Process>(2, "P2", 0, 5));
    processes.push_back(std::make_shared<Process>(3, "P3", 0, 8));

    scheduler.add_process(processes[0]);
    scheduler.add_process(processes[1]);
    scheduler.add_process(processes[2]);

    scheduler.remove_process(2);

    REQUIRE(scheduler.size() == 2);
    REQUIRE(scheduler.get_next_process()->pid == 3);
  }

  SECTION("Clear scheduler") {
    processes.clear();
    processes.push_back(std::make_shared<Process>(1, "P1", 0, 10));
    processes.push_back(std::make_shared<Process>(2, "P2", 0, 5));

    scheduler.add_process(processes[0]);
    scheduler.add_process(processes[1]);

    scheduler.clear();
    REQUIRE(scheduler.size() == 0);
    REQUIRE_FALSE(scheduler.has_processes());
  }
}
