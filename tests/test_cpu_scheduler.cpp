#include "core/process.hpp"
#include "cpu/cpu_scheduler.hpp"
#include "cpu/fcfs_scheduler.hpp"
#include "cpu/priority_scheduler.hpp"
#include "cpu/round_robin_scheduler.hpp"
#include "cpu/sjf_scheduler.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using namespace OSSimulator;

TEST_CASE("CPU Scheduler - FCFS Integration", "[cpu_scheduler][fcfs]") {
  CPUScheduler cpu_scheduler;
  cpu_scheduler.set_scheduler(std::make_unique<FCFSScheduler>());

  SECTION("Single process execution") {
    Process p1(1, "P1", 0, 5);
    cpu_scheduler.add_process(p1);

    cpu_scheduler.run_until_completion();

    auto completed = cpu_scheduler.get_completed_processes();
    REQUIRE(completed.size() == 1);
    REQUIRE(completed[0].pid == 1);
    REQUIRE(completed[0].completion_time == 5);
    REQUIRE(completed[0].waiting_time == 0);
    REQUIRE(completed[0].turnaround_time == 5);
  }

  SECTION("Multiple processes - no waiting") {
    Process p1(1, "P1", 0, 5);
    Process p2(2, "P2", 0, 3);
    Process p3(3, "P3", 0, 4);

    cpu_scheduler.add_process(p1);
    cpu_scheduler.add_process(p2);
    cpu_scheduler.add_process(p3);

    cpu_scheduler.run_until_completion();

    auto completed = cpu_scheduler.get_completed_processes();
    REQUIRE(completed.size() == 3);
    REQUIRE(cpu_scheduler.get_current_time() == 12);
  }

  SECTION("Processes with different arrival times") {
    Process p1(1, "P1", 0, 4);
    Process p2(2, "P2", 1, 3);
    Process p3(3, "P3", 2, 2);

    cpu_scheduler.add_process(p1);
    cpu_scheduler.add_process(p2);
    cpu_scheduler.add_process(p3);

    cpu_scheduler.run_until_completion();

    auto completed = cpu_scheduler.get_completed_processes();
    REQUIRE(completed.size() == 3);
    REQUIRE(completed[0].pid == 1);
    REQUIRE(completed[0].completion_time == 4);
  }
}

TEST_CASE("CPU Scheduler - SJF Integration", "[cpu_scheduler][sjf]") {
  CPUScheduler cpu_scheduler;
  cpu_scheduler.set_scheduler(std::make_unique<SJFScheduler>());

  SECTION("Shortest job first order") {
    Process p1(1, "P1", 0, 8);
    Process p2(2, "P2", 0, 4);
    Process p3(3, "P3", 0, 2);

    cpu_scheduler.add_process(p1);
    cpu_scheduler.add_process(p2);
    cpu_scheduler.add_process(p3);

    cpu_scheduler.run_until_completion();

    auto completed = cpu_scheduler.get_completed_processes();
    REQUIRE(completed.size() == 3);
    REQUIRE(completed[0].pid == 3);
    REQUIRE(completed[1].pid == 2);
    REQUIRE(completed[2].pid == 1);
  }

  SECTION("Calculate metrics correctly") {
    Process p1(1, "P1", 0, 6);
    Process p2(2, "P2", 0, 2);
    Process p3(3, "P3", 0, 8);

    cpu_scheduler.add_process(p1);
    cpu_scheduler.add_process(p2);
    cpu_scheduler.add_process(p3);

    cpu_scheduler.run_until_completion();

    auto completed = cpu_scheduler.get_completed_processes();
    REQUIRE(completed[0].pid == 2);
    REQUIRE(completed[0].waiting_time == 0);
    REQUIRE(completed[0].turnaround_time == 2);

    REQUIRE(completed[1].pid == 1);
    REQUIRE(completed[1].waiting_time == 2);
    REQUIRE(completed[1].turnaround_time == 8);
  }
}

TEST_CASE("CPU Scheduler - Round Robin Integration",
          "[cpu_scheduler][round_robin]") {
  CPUScheduler cpu_scheduler;
  auto rr = std::make_unique<RoundRobinScheduler>(2);
  cpu_scheduler.set_scheduler(std::move(rr));

  SECTION("Simple round robin with quantum 2") {
    Process p1(1, "P1", 0, 5);
    Process p2(2, "P2", 0, 3);

    cpu_scheduler.add_process(p1);
    cpu_scheduler.add_process(p2);

    cpu_scheduler.run_until_completion();

    auto completed = cpu_scheduler.get_completed_processes();
    REQUIRE(completed.size() == 2);
    REQUIRE(cpu_scheduler.get_current_time() == 8);
  }

  SECTION("Context switches occur") {
    Process p1(1, "P1", 0, 6);
    Process p2(2, "P2", 0, 4);
    Process p3(3, "P3", 0, 2);

    cpu_scheduler.add_process(p1);
    cpu_scheduler.add_process(p2);
    cpu_scheduler.add_process(p3);

    cpu_scheduler.run_until_completion();

    REQUIRE(cpu_scheduler.get_context_switches() > 0);
  }

  SECTION("Different quantum values") {
    CPUScheduler cpu_scheduler2;
    auto rr2 = std::make_unique<RoundRobinScheduler>(4);
    cpu_scheduler2.set_scheduler(std::move(rr2));

    Process p1(1, "P1", 0, 10);
    Process p2(2, "P2", 0, 10);

    cpu_scheduler2.add_process(p1);
    cpu_scheduler2.add_process(p2);

    cpu_scheduler2.run_until_completion();

    auto completed = cpu_scheduler2.get_completed_processes();
    REQUIRE(completed.size() == 2);
    REQUIRE(cpu_scheduler2.get_current_time() == 20);
  }
}

TEST_CASE("CPU Scheduler - Priority Integration", "[cpu_scheduler][priority]") {
  CPUScheduler cpu_scheduler;
  cpu_scheduler.set_scheduler(std::make_unique<PriorityScheduler>());

  SECTION("Higher priority executes first") {
    Process p1(1, "P1", 0, 5, 3);
    Process p2(2, "P2", 0, 3, 1);
    Process p3(3, "P3", 0, 4, 2);

    cpu_scheduler.add_process(p1);
    cpu_scheduler.add_process(p2);
    cpu_scheduler.add_process(p3);

    cpu_scheduler.run_until_completion();

    auto completed = cpu_scheduler.get_completed_processes();
    REQUIRE(completed.size() == 3);
    REQUIRE(completed[0].pid == 2);
    REQUIRE(completed[1].pid == 3);
    REQUIRE(completed[2].pid == 1);
  }

  SECTION("Same priority - FCFS") {
    Process p1(1, "P1", 0, 5, 2);
    Process p2(2, "P2", 1, 3, 2);
    Process p3(3, "P3", 2, 4, 2);

    cpu_scheduler.add_process(p1);
    cpu_scheduler.add_process(p2);
    cpu_scheduler.add_process(p3);

    cpu_scheduler.run_until_completion();

    auto completed = cpu_scheduler.get_completed_processes();
    REQUIRE(completed.size() == 3);
    REQUIRE(completed[0].pid == 1);
    REQUIRE(completed[1].pid == 2);
    REQUIRE(completed[2].pid == 3);
  }
}

TEST_CASE("CPU Scheduler - Metrics Calculation", "[cpu_scheduler][metrics]") {
  CPUScheduler cpu_scheduler;
  cpu_scheduler.set_scheduler(std::make_unique<FCFSScheduler>());

  SECTION("Average waiting time") {
    Process p1(1, "P1", 0, 5);
    Process p2(2, "P2", 0, 3);
    Process p3(3, "P3", 0, 2);

    cpu_scheduler.add_process(p1);
    cpu_scheduler.add_process(p2);
    cpu_scheduler.add_process(p3);

    cpu_scheduler.run_until_completion();

    double avg_wait = cpu_scheduler.get_average_waiting_time();
    REQUIRE_THAT(avg_wait, Catch::Matchers::WithinRel(4.33, 0.01));
  }

  SECTION("Average turnaround time") {
    Process p1(1, "P1", 0, 5);
    Process p2(2, "P2", 0, 3);

    cpu_scheduler.add_process(p1);
    cpu_scheduler.add_process(p2);

    cpu_scheduler.run_until_completion();

    double avg_turnaround = cpu_scheduler.get_average_turnaround_time();
    REQUIRE(avg_turnaround > 0);
  }

  SECTION("Average response time") {
    Process p1(1, "P1", 0, 5);
    Process p2(2, "P2", 0, 3);

    cpu_scheduler.add_process(p1);
    cpu_scheduler.add_process(p2);

    cpu_scheduler.run_until_completion();

    double avg_response = cpu_scheduler.get_average_response_time();
    REQUIRE(avg_response >= 0);
  }
}

TEST_CASE("CPU Scheduler - Process States", "[cpu_scheduler][states]") {
  CPUScheduler cpu_scheduler;
  cpu_scheduler.set_scheduler(std::make_unique<FCFSScheduler>());

  SECTION("Process transitions through states") {
    Process p1(1, "P1", 0, 3);
    cpu_scheduler.add_process(p1);

    cpu_scheduler.run_until_completion();

    auto completed = cpu_scheduler.get_completed_processes();
    REQUIRE(completed[0].state == ProcessState::TERMINATED);
  }
}

TEST_CASE("CPU Scheduler - Reset Functionality", "[cpu_scheduler][reset]") {
  CPUScheduler cpu_scheduler;
  cpu_scheduler.set_scheduler(std::make_unique<FCFSScheduler>());

  Process p1(1, "P1", 0, 5);
  Process p2(2, "P2", 0, 3);

  cpu_scheduler.add_process(p1);
  cpu_scheduler.add_process(p2);

  cpu_scheduler.run_until_completion();

  REQUIRE(cpu_scheduler.get_completed_processes().size() == 2);

  cpu_scheduler.reset();

  REQUIRE(cpu_scheduler.get_current_time() == 0);
  REQUIRE(cpu_scheduler.get_context_switches() == 0);
  REQUIRE(cpu_scheduler.get_completed_processes().size() == 0);
}

TEST_CASE("CPU Scheduler - Arrival Time Handling", "[cpu_scheduler][arrival]") {
  CPUScheduler cpu_scheduler;
  cpu_scheduler.set_scheduler(std::make_unique<FCFSScheduler>());

  SECTION("Late arriving processes") {
    Process p1(1, "P1", 0, 2);
    Process p2(2, "P2", 5, 3);
    Process p3(3, "P3", 10, 2);

    cpu_scheduler.add_process(p1);
    cpu_scheduler.add_process(p2);
    cpu_scheduler.add_process(p3);

    cpu_scheduler.run_until_completion();

    auto completed = cpu_scheduler.get_completed_processes();
    REQUIRE(completed.size() == 3);
    REQUIRE(cpu_scheduler.get_current_time() >= 10);
  }
}

TEST_CASE("CPU Scheduler - Context Switch Counting",
          "[cpu_scheduler][context_switch]") {
  SECTION("FCFS minimal context switches") {
    CPUScheduler cpu_scheduler;
    cpu_scheduler.set_scheduler(std::make_unique<FCFSScheduler>());

    Process p1(1, "P1", 0, 5);
    Process p2(2, "P2", 0, 3);

    cpu_scheduler.add_process(p1);
    cpu_scheduler.add_process(p2);

    cpu_scheduler.run_until_completion();

    REQUIRE(cpu_scheduler.get_context_switches() >= 1);
  }

  SECTION("Round Robin multiple context switches") {
    CPUScheduler cpu_scheduler;
    auto rr = std::make_unique<RoundRobinScheduler>(1);
    cpu_scheduler.set_scheduler(std::move(rr));

    Process p1(1, "P1", 0, 3);
    Process p2(2, "P2", 0, 3);

    cpu_scheduler.add_process(p1);
    cpu_scheduler.add_process(p2);

    cpu_scheduler.run_until_completion();

    REQUIRE(cpu_scheduler.get_context_switches() >= 2);
  }
}

TEST_CASE("CPU Scheduler - Memory Callback", "[cpu_scheduler][memory]") {
  CPUScheduler cpu_scheduler;
  cpu_scheduler.set_scheduler(std::make_unique<FCFSScheduler>());

  SECTION("Memory callback integration") {
    bool callback_called = false;

    cpu_scheduler.set_memory_callback([&callback_called](const Process &) {
      callback_called = true;
      return true;
    });

    Process p1(1, "P1", 0, 5);
    cpu_scheduler.add_process(p1);

    cpu_scheduler.run_until_completion();

    REQUIRE(callback_called == true);
  }
}
