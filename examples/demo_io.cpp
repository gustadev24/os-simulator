#include "core/burst.hpp"
#include "core/process.hpp"
#include "io/io_device.hpp"
#include "io/io_fcfs_scheduler.hpp"
#include "io/io_manager.hpp"
#include "io/io_round_robin_scheduler.hpp"
#include <iomanip>
#include <iostream>
#include <memory>
#include <vector>

using namespace OSSimulator;

void print_header(const std::string &title) {
  std::cout << "\n========================================\n";
  std::cout << "  " << title << "\n";
  std::cout << "========================================\n\n";
}

void demo_io_fcfs() {
  print_header("I/O Scheduling - FCFS");

  IODevice disk("disk");
  disk.set_scheduler(std::make_unique<IOFCFSScheduler>());

  std::cout << "Device: " << disk.get_device_name() << "\n";
  std::cout << "Scheduling Algorithm: FCFS\n\n";

  int completed_count = 0;
  disk.set_completion_callback([&](std::shared_ptr<Process> proc, int time) {
    completed_count++;
    std::cout << "  [Time " << time << "] Process " << proc->name
              << " completed I/O\n";
  });

  auto p1 = std::make_shared<Process>(1, "P1", 0, 10);
  auto p2 = std::make_shared<Process>(2, "P2", 1, 8);
  auto p3 = std::make_shared<Process>(3, "P3", 2, 6);

  auto req1 =
      std::make_shared<IORequest>(p1, Burst(BurstType::IO, 5, "disk"), 0);
  auto req2 =
      std::make_shared<IORequest>(p2, Burst(BurstType::IO, 3, "disk"), 1);
  auto req3 =
      std::make_shared<IORequest>(p3, Burst(BurstType::IO, 4, "disk"), 2);

  std::cout << "Submitting I/O requests:\n";
  std::cout << "  P1: 5 time units\n";
  std::cout << "  P2: 3 time units\n";
  std::cout << "  P3: 4 time units\n\n";

  disk.add_io_request(req1);
  disk.add_io_request(req2);
  disk.add_io_request(req3);

  std::cout << "Executing I/O operations:\n";

  int current_time = 0;
  while (disk.has_pending_requests()) {
    disk.execute_step(0, current_time);
    if (disk.has_pending_requests()) {
      current_time++;
    }
  }

  std::cout << "\nResults:\n";
  std::cout << "  Total I/O time: " << disk.get_total_io_time() << "\n";
  std::cout << "  Device switches: " << disk.get_device_switches() << "\n";
  std::cout << "  Requests completed: " << disk.get_total_requests_completed()
            << "\n";
}

void demo_io_round_robin() {
  print_header("I/O Scheduling - Round Robin (Quantum = 4)");

  IODevice disk("disk");
  disk.set_scheduler(std::make_unique<IORoundRobinScheduler>(4));

  std::cout << "Device: " << disk.get_device_name() << "\n";
  std::cout << "Scheduling Algorithm: Round Robin (Quantum = 4)\n\n";

  int completed_count = 0;
  disk.set_completion_callback([&](std::shared_ptr<Process> proc, int time) {
    completed_count++;
    std::cout << "  [Time " << time << "] Process " << proc->name
              << " completed I/O\n";
  });

  auto p1 = std::make_shared<Process>(1, "P1", 0, 10);
  auto p2 = std::make_shared<Process>(2, "P2", 1, 8);
  auto p3 = std::make_shared<Process>(3, "P3", 2, 6);

  auto req1 =
      std::make_shared<IORequest>(p1, Burst(BurstType::IO, 10, "disk"), 0);
  auto req2 =
      std::make_shared<IORequest>(p2, Burst(BurstType::IO, 6, "disk"), 1);
  auto req3 =
      std::make_shared<IORequest>(p3, Burst(BurstType::IO, 8, "disk"), 2);

  std::cout << "Submitting I/O requests:\n";
  std::cout << "  P1: 10 time units\n";
  std::cout << "  P2: 6 time units\n";
  std::cout << "  P3: 8 time units\n\n";

  disk.add_io_request(req1);
  disk.add_io_request(req2);
  disk.add_io_request(req3);

  std::cout << "Executing I/O operations with Round Robin:\n";

  int current_time = 0;
  int step = 0;
  while (disk.has_pending_requests()) {
    step++;
    std::cout << "  [Step " << step << ", Time " << current_time
              << "] Queue size: " << disk.get_queue_size() << "\n";

    int time_before = current_time;
    disk.execute_step(4, current_time);

    if (disk.is_busy() || disk.has_pending_requests()) {
      current_time += 4;
    }
  }

  std::cout << "\nResults:\n";
  std::cout << "  Total I/O time: " << disk.get_total_io_time() << "\n";
  std::cout << "  Device switches: " << disk.get_device_switches() << "\n";
  std::cout << "  Requests completed: " << disk.get_total_requests_completed()
            << "\n";
  std::cout << "  Total time: " << current_time << "\n";
}

void demo_io_manager() {
  print_header("I/O Manager - Multiple Devices");

  IOManager manager;

  auto disk = std::make_shared<IODevice>("disk");
  disk->set_scheduler(std::make_unique<IORoundRobinScheduler>(4));

  auto tape = std::make_shared<IODevice>("tape");
  tape->set_scheduler(std::make_unique<IOFCFSScheduler>());

  manager.add_device("disk", disk);
  manager.add_device("tape", tape);

  std::cout << "Devices registered:\n";
  std::cout << "  - disk (Round Robin, quantum=4)\n";
  std::cout << "  - tape (FCFS)\n\n";

  int completed_count = 0;
  manager.set_completion_callback([&](std::shared_ptr<Process> proc, int time) {
    completed_count++;
    std::cout << "  [Time " << time << "] Process " << proc->name
              << " completed I/O\n";
  });

  auto p1 = std::make_shared<Process>(1, "P1", 0, 10);
  auto p2 = std::make_shared<Process>(2, "P2", 1, 8);
  auto p3 = std::make_shared<Process>(3, "P3", 2, 6);

  std::cout << "Submitting requests to different devices:\n";

  auto req1 =
      std::make_shared<IORequest>(p1, Burst(BurstType::IO, 8, "disk"), 0);
  auto req2 =
      std::make_shared<IORequest>(p2, Burst(BurstType::IO, 5, "tape"), 1);
  auto req3 =
      std::make_shared<IORequest>(p3, Burst(BurstType::IO, 6, "disk"), 2);

  std::cout << "  P1 -> disk: 8 time units\n";
  std::cout << "  P2 -> tape: 5 time units\n";
  std::cout << "  P3 -> disk: 6 time units\n\n";

  manager.submit_io_request(req1);
  manager.submit_io_request(req2);
  manager.submit_io_request(req3);

  std::cout << "Executing all devices concurrently:\n";

  int current_time = 0;
  while (manager.has_pending_io()) {
    manager.execute_all_devices(4, current_time);
    current_time += 4;
  }

  std::cout << "\nResults:\n";
  std::cout << "  Requests completed: " << completed_count << "\n";
  std::cout << "  Total time: " << current_time << "\n";

  auto devices = manager.get_all_devices();
  for (const auto &[name, dev] : devices) {
    std::cout << "  Device '" << name
              << "': Total I/O time=" << dev->get_total_io_time()
              << ", Switches=" << dev->get_device_switches() << "\n";
  }
}

void demo_process_with_bursts() {
  print_header("Process with CPU and I/O Bursts");

  std::vector<Burst> bursts = {
      Burst(BurstType::CPU, 4), Burst(BurstType::IO, 3, "disk"),
      Burst(BurstType::CPU, 5), Burst(BurstType::IO, 2, "disk"),
      Burst(BurstType::CPU, 3)};

  auto proc = std::make_shared<Process>(1, "P1", 0, bursts, 0);

  std::cout << "Process: " << proc->name << "\n";
  std::cout << "Burst sequence:\n";

  for (size_t i = 0; i < proc->burst_sequence.size(); ++i) {
    const auto &burst = proc->burst_sequence[i];
    std::cout << "  [" << i << "] "
              << (burst.type == BurstType::CPU ? "CPU" : "I/O") << ": "
              << burst.duration << " units";
    if (burst.type == BurstType::IO) {
      std::cout << " (device: " << burst.io_device << ")";
    }
    std::cout << "\n";
  }

  std::cout << "\nTotal CPU time: " << proc->total_cpu_time << "\n";
  std::cout << "Total I/O time: " << proc->total_io_time << "\n";
  std::cout << "Total burst time: " << proc->get_total_burst_time() << "\n";

  std::cout << "\nSimulating burst execution:\n";

  int current_time = 0;
  while (proc->has_more_bursts()) {
    const Burst *current_burst = proc->get_current_burst();
    if (!current_burst)
      break;

    std::cout << "  [Time " << current_time << "] Executing "
              << (current_burst->type == BurstType::CPU ? "CPU" : "I/O")
              << " burst (" << current_burst->duration << " units)\n";

    if (current_burst->type == BurstType::CPU) {
      int executed = proc->execute(0, current_time);
      current_time += executed;
    } else {
      current_time += current_burst->duration;
      proc->advance_to_next_burst();
    }
  }

  std::cout << "  [Time " << current_time << "] Process completed\n";
}

int main() {
  std::cout << "\n";
  std::cout << "╔══════════════════════════════════════════════════╗\n";
  std::cout << "║   I/O Burst Scheduling Demonstration             ║\n";
  std::cout << "║   Operating System Simulator                     ║\n";
  std::cout << "╚══════════════════════════════════════════════════╝\n";

  demo_process_with_bursts();
  demo_io_fcfs();
  demo_io_round_robin();
  demo_io_manager();

  std::cout << "\n========================================\n";
  std::cout << "  Demonstration Complete\n";
  std::cout << "========================================\n\n";

  return 0;
}
