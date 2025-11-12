#include "core/process.hpp"
#include "cpu/priority_scheduler.hpp"
#include <catch2/catch_test_macros.hpp>
#include <vector>

using namespace OSSimulator;

// TEST_CASE("Priority Scheduler - Basic Operations", "[priority]") {
//   PriorityScheduler scheduler;
//   std::vector<Process> processes;

//   SECTION("Empty scheduler") {
//     REQUIRE_FALSE(scheduler.has_processes());
//     REQUIRE(scheduler.size() == 0);
//     REQUIRE(scheduler.get_next_process() == nullptr);
//   }

//   SECTION("Add single process") {
//     processes.emplace_back(1, "P1", 0, 10, 5);
//     scheduler.add_process(&processes[0]);

//     REQUIRE(scheduler.has_processes());
//     REQUIRE(scheduler.size() == 1);
//     REQUIRE(scheduler.get_next_process() != nullptr);
//     REQUIRE(scheduler.get_next_process()->pid == 1);
//   }

//   SECTION("Algorithm type") {
//     REQUIRE(scheduler.get_algorithm() == SchedulingAlgorithm::PRIORITY);
//   }
// }

// TEST_CASE("Priority Scheduler - Priority Order", "[priority]") {
//   PriorityScheduler scheduler;
//   std::vector<Process> processes;

//   SECTION("Lower priority number = higher priority") {
//     processes.emplace_back(1, "P1", 0, 10, 5);
//     processes.emplace_back(2, "P2", 0, 5, 1);
//     processes.emplace_back(3, "P3", 0, 8, 3);

//     scheduler.add_process(&processes[0]);
//     scheduler.add_process(&processes[1]);
//     scheduler.add_process(&processes[2]);

//     REQUIRE(scheduler.get_next_process()->pid == 2);
//     scheduler.remove_process(2);

//     REQUIRE(scheduler.get_next_process()->pid == 3);
//     scheduler.remove_process(3);

//     REQUIRE(scheduler.get_next_process()->pid == 1);
//   }

//   SECTION("Equal priority - arrival time breaks tie") {
//     processes.clear();
//     processes.emplace_back(1, "P1", 2, 10, 3);
//     processes.emplace_back(2, "P2", 0, 5, 3);
//     processes.emplace_back(3, "P3", 1, 8, 3);

//     scheduler.add_process(&processes[0]);
//     scheduler.add_process(&processes[1]);
//     scheduler.add_process(&processes[2]);

//     REQUIRE(scheduler.get_next_process()->pid == 2);
//     scheduler.remove_process(2);

//     REQUIRE(scheduler.get_next_process()->pid == 3);
//     scheduler.remove_process(3);

//     REQUIRE(scheduler.get_next_process()->pid == 1);
//   }

//   SECTION("Mixed priorities") {
//     processes.clear();
//     processes.emplace_back(1, "P1", 0, 10, 0);
//     processes.emplace_back(2, "P2", 0, 5, 10);
//     processes.emplace_back(3, "P3", 0, 8, 5);
//     processes.emplace_back(4, "P4", 0, 3, 2);

//     scheduler.add_process(&processes[0]);
//     scheduler.add_process(&processes[1]);
//     scheduler.add_process(&processes[2]);
//     scheduler.add_process(&processes[3]);

//     REQUIRE(scheduler.get_next_process()->pid == 1);
//     scheduler.remove_process(1);

//     REQUIRE(scheduler.get_next_process()->pid == 4);
//     scheduler.remove_process(4);

//     REQUIRE(scheduler.get_next_process()->pid == 3);
//     scheduler.remove_process(3);

//     REQUIRE(scheduler.get_next_process()->pid == 2);
//   }
// }

// TEST_CASE("Priority Scheduler - Remove and Clear", "[priority]") {
//   PriorityScheduler scheduler;
//   std::vector<Process> processes;

//   SECTION("Remove process maintains order") {
//     processes.emplace_back(1, "P1", 0, 10, 5);
//     processes.emplace_back(2, "P2", 0, 5, 1);
//     processes.emplace_back(3, "P3", 0, 8, 3);

//     scheduler.add_process(&processes[0]);
//     scheduler.add_process(&processes[1]);
//     scheduler.add_process(&processes[2]);

//     scheduler.remove_process(2);

//     REQUIRE(scheduler.size() == 2);
//     REQUIRE(scheduler.get_next_process()->pid == 3);
//   }

//   SECTION("Clear scheduler") {
//     processes.clear();
//     processes.emplace_back(1, "P1", 0, 10, 5);
//     processes.emplace_back(2, "P2", 0, 5, 1);

//     scheduler.add_process(&processes[0]);
//     scheduler.add_process(&processes[1]);

//     scheduler.clear();
//     REQUIRE(scheduler.size() == 0);
//     REQUIRE_FALSE(scheduler.has_processes());
//   }
// }

// TEST_CASE("Priority Scheduler - Edge Cases", "[priority]") {
//   PriorityScheduler scheduler;
//   std::vector<Process> processes;

//   SECTION("All same priority and arrival time") {
//     processes.emplace_back(1, "P1", 0, 10, 5);
//     processes.emplace_back(2, "P2", 0, 5, 5);
//     processes.emplace_back(3, "P3", 0, 8, 5);

//     scheduler.add_process(&processes[0]);
//     scheduler.add_process(&processes[1]);
//     scheduler.add_process(&processes[2]);

//     REQUIRE(scheduler.get_next_process()->pid == 1);
//     scheduler.remove_process(1);

//     REQUIRE(scheduler.get_next_process()->pid == 2);
//     scheduler.remove_process(2);

//     REQUIRE(scheduler.get_next_process()->pid == 3);
//   }

//   SECTION("Priority 0 is highest") {
//     processes.clear();
//     processes.emplace_back(1, "P1", 0, 10, 0);
//     processes.emplace_back(2, "P2", 0, 5, 1);

//     scheduler.add_process(&processes[1]);
//     scheduler.add_process(&processes[0]);

//     REQUIRE(scheduler.get_next_process()->pid == 1);
//   }
// }
