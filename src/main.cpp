#include "core/process.hpp"
#include "cpu/cpu_scheduler.hpp"
#include "cpu/fcfs_scheduler.hpp"
#include "cpu/priority_scheduler.hpp"
#include "cpu/round_robin_scheduler.hpp"
#include "cpu/sjf_scheduler.hpp"
#include <iomanip>
#include <iostream>
#include <memory>
#include <vector>

using namespace OSSimulator;

void print_header(const std::string &algorithm) {
  std::cout << "\n========================================\n";
  std::cout << "  " << algorithm << " Scheduling\n";
  std::cout << "========================================\n\n";
}

void print_results(CPUScheduler &scheduler) {
  auto completed = scheduler.get_completed_processes();

  std::cout << std::left << std::setw(6) << "PID" << std::setw(12) << "Name"
            << std::setw(10) << "Arrival" << std::setw(10) << "Burst"
            << std::setw(12) << "Completion" << std::setw(10) << "Waiting"
            << std::setw(12) << "Turnaround" << std::setw(10) << "Response"
            << "\n";
  std::cout << std::string(80, '-') << "\n";

  for (const auto &proc : completed) {
    std::cout << std::left << std::setw(6) << proc.pid << std::setw(12)
              << proc.name << std::setw(10) << proc.arrival_time
              << std::setw(10) << proc.burst_time << std::setw(12)
              << proc.completion_time << std::setw(10) << proc.waiting_time
              << std::setw(12) << proc.turnaround_time << std::setw(10)
              << proc.response_time << "\n";
  }

  std::cout << "\n";
  std::cout << "Average Waiting Time:    " << std::fixed << std::setprecision(2)
            << scheduler.get_average_waiting_time() << "\n";
  std::cout << "Average Turnaround Time: "
            << scheduler.get_average_turnaround_time() << "\n";
  std::cout << "Average Response Time:   "
            << scheduler.get_average_response_time() << "\n";
  std::cout << "Context Switches:        " << scheduler.get_context_switches()
            << "\n";
  std::cout << "Total Time:              " << scheduler.get_current_time()
            << "\n";
}

void demo_fcfs() {
  print_header("FCFS (First Come First Served)");

  CPUScheduler scheduler;
  scheduler.set_scheduler(std::make_unique<FCFSScheduler>());

  std::vector<Process> processes = {
      Process(1, "P1", 0, 8), Process(2, "P2", 1, 4), Process(3, "P3", 2, 9),
      Process(4, "P4", 3, 5)};

  scheduler.load_processes(processes);
  scheduler.run_until_completion();

  print_results(scheduler);
}

void demo_sjf() {
  print_header("SJF (Shortest Job First)");

  CPUScheduler scheduler;
  scheduler.set_scheduler(std::make_unique<SJFScheduler>());

  std::vector<Process> processes = {
      Process(1, "P1", 0, 8), Process(2, "P2", 1, 4), Process(3, "P3", 2, 2),
      Process(4, "P4", 3, 1)};

  scheduler.load_processes(processes);
  scheduler.run_until_completion();

  print_results(scheduler);
}

void demo_round_robin() {
  print_header("Round Robin (Quantum = 4)");

  CPUScheduler scheduler;
  auto rr_scheduler = std::make_unique<RoundRobinScheduler>(4);
  scheduler.set_scheduler(std::move(rr_scheduler));

  std::vector<Process> processes = {
      Process(1, "P1", 0, 10), Process(2, "P2", 1, 8), Process(3, "P3", 2, 6),
      Process(4, "P4", 3, 4)};

  scheduler.load_processes(processes);
  scheduler.run_until_completion();

  print_results(scheduler);
}

void demo_priority() {
  print_header("Priority Scheduling (Lower number = Higher priority)");

  CPUScheduler scheduler;
  scheduler.set_scheduler(std::make_unique<PriorityScheduler>());

  std::vector<Process> processes = {
      Process(1, "P1", 0, 8, 3), Process(2, "P2", 1, 4, 1),
      Process(3, "P3", 2, 9, 4), Process(4, "P4", 3, 5, 2)};

  scheduler.load_processes(processes);
  scheduler.run_until_completion();

  print_results(scheduler);
}

int main() {
  std::cout << "\n";
  std::cout << "╔══════════════════════════════════════════════════╗\n";
  std::cout << "║   CPU Scheduling Algorithms Demonstration        ║\n";
  std::cout << "║   Operating System Simulator                     ║\n";
  std::cout << "╚══════════════════════════════════════════════════╝\n";

  demo_fcfs();
  demo_sjf();
  demo_round_robin();
  demo_priority();

  std::cout << "\n========================================\n";
  std::cout << "  Demonstration Complete\n";
  std::cout << "========================================\n\n";

  return 0;
}
