#include "core/process.hpp"
#include "cpu/sjf_scheduler.hpp"
#include <catch2/catch_test_macros.hpp>
#include <vector>

using namespace OSSimulator;

// TEST_CASE("SJF Scheduler - Basic Operations", "[sjf]") {
//   SJFScheduler scheduler;
//   std::vector<Process> processes;

//   SECTION("Empty scheduler") {
//     REQUIRE_FALSE(scheduler.has_processes());
//     REQUIRE(scheduler.size() == 0);
//     REQUIRE(scheduler.get_next_process() == nullptr);
//   }

//   SECTION("Add single process") {
//     processes.emplace_back(1, "P1", 0, 10);
//     scheduler.add_process(&processes[0]);

//     REQUIRE(scheduler.has_processes());
//     REQUIRE(scheduler.size() == 1);
//     REQUIRE(scheduler.get_next_process() != nullptr);
//     REQUIRE(scheduler.get_next_process()->pid == 1);
//   }

//   SECTION("Algorithm type") {
//     REQUIRE(scheduler.get_algorithm() == SchedulingAlgorithm::SJF);
//   }
// }

// TEST_CASE("SJF Scheduler - Shortest Job First Order", "[sjf]") {
//   SJFScheduler scheduler;
//   std::vector<Process> processes;

//   SECTION("Processes sorted by burst time") {
//     processes.emplace_back(1, "P1", 0, 10);
//     processes.emplace_back(2, "P2", 0, 5);
//     processes.emplace_back(3, "P3", 0, 8);

//     scheduler.add_process(&processes[0]);
//     scheduler.add_process(&processes[1]);
//     scheduler.add_process(&processes[2]);

//     REQUIRE(scheduler.get_next_process()->pid == 2);
//     scheduler.remove_process(2);

//     REQUIRE(scheduler.get_next_process()->pid == 3);
//     scheduler.remove_process(3);

//     REQUIRE(scheduler.get_next_process()->pid == 1);
//   }

//   SECTION("Equal burst time - arrival time breaks tie") {
//     processes.clear();
//     processes.emplace_back(1, "P1", 2, 5);
//     processes.emplace_back(2, "P2", 0, 5);
//     processes.emplace_back(3, "P3", 1, 5);

//     scheduler.add_process(&processes[0]);
//     scheduler.add_process(&processes[1]);
//     scheduler.add_process(&processes[2]);

//     REQUIRE(scheduler.get_next_process()->pid == 2);
//     scheduler.remove_process(2);

//     REQUIRE(scheduler.get_next_process()->pid == 3);
//     scheduler.remove_process(3);

//     REQUIRE(scheduler.get_next_process()->pid == 1);
//   }

//   SECTION("Dynamic sorting with remaining time") {
//     processes.clear();
//     processes.emplace_back(1, "P1", 0, 10);
//     processes[0].remaining_time = 3;

//     processes.emplace_back(2, "P2", 0, 5);
//     processes[1].remaining_time = 5;

//     processes.emplace_back(3, "P3", 0, 8);
//     processes[2].remaining_time = 2;

//     scheduler.add_process(&processes[0]);
//     scheduler.add_process(&processes[1]);
//     scheduler.add_process(&processes[2]);

//     REQUIRE(scheduler.get_next_process()->pid == 3);
//     scheduler.remove_process(3);

//     REQUIRE(scheduler.get_next_process()->pid == 1);
//     scheduler.remove_process(1);

//     REQUIRE(scheduler.get_next_process()->pid == 2);
//   }
// }

// TEST_CASE("SJF Scheduler - Remove and Clear", "[sjf]") {
//   SJFScheduler scheduler;
//   std::vector<Process> processes;

//   SECTION("Remove process maintains order") {
//     processes.emplace_back(1, "P1", 0, 10);
//     processes.emplace_back(2, "P2", 0, 5);
//     processes.emplace_back(3, "P3", 0, 8);

//     scheduler.add_process(&processes[0]);
//     scheduler.add_process(&processes[1]);
//     scheduler.add_process(&processes[2]);

//     scheduler.remove_process(2);

//     REQUIRE(scheduler.size() == 2);
//     REQUIRE(scheduler.get_next_process()->pid == 3);
//   }

//   SECTION("Clear scheduler") {
//     processes.clear();
//     processes.emplace_back(1, "P1", 0, 10);
//     processes.emplace_back(2, "P2", 0, 5);

//     scheduler.add_process(&processes[0]);
//     scheduler.add_process(&processes[1]);

//     scheduler.clear();
//     REQUIRE(scheduler.size() == 0);
//     REQUIRE_FALSE(scheduler.has_processes());
//   }
// }
