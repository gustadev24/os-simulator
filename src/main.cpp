#include "core/config_parser.hpp"
#include "core/process.hpp"
#include "cpu/cpu_scheduler.hpp"
#include "cpu/fcfs_scheduler.hpp"
#include "cpu/priority_scheduler.hpp"
#include "cpu/round_robin_scheduler.hpp"
#include "cpu/sjf_scheduler.hpp"
#include <cstring>
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
    std::cout << std::left << std::setw(6) << proc->pid << std::setw(12)
              << proc->name << std::setw(10) << proc->arrival_time
              << std::setw(10) << proc->burst_time << std::setw(12)
              << proc->completion_time << std::setw(10) << proc->waiting_time
              << std::setw(12) << proc->turnaround_time << std::setw(10)
              << proc->response_time << "\n";
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

  std::vector<std::shared_ptr<Process>> processes = {
      std::make_shared<Process>(1, "P1", 0, 8),
      std::make_shared<Process>(2, "P2", 1, 4),
      std::make_shared<Process>(3, "P3", 2, 9),
      std::make_shared<Process>(4, "P4", 3, 5)};

  scheduler.load_processes(processes);
  scheduler.run_until_completion();

  print_results(scheduler);
}

void demo_sjf() {
  print_header("SJF (Shortest Job First)");

  CPUScheduler scheduler;
  scheduler.set_scheduler(std::make_unique<SJFScheduler>());

  std::vector<std::shared_ptr<Process>> processes = {
      std::make_shared<Process>(1, "P1", 0, 8),
      std::make_shared<Process>(2, "P2", 1, 4),
      std::make_shared<Process>(3, "P3", 2, 2),
      std::make_shared<Process>(4, "P4", 3, 1)};

  scheduler.load_processes(processes);
  scheduler.run_until_completion();

  print_results(scheduler);
}

void demo_round_robin() {
  print_header("Round Robin (Quantum = 4)");

  CPUScheduler scheduler;
  auto rr_scheduler = std::make_unique<RoundRobinScheduler>(4);
  scheduler.set_scheduler(std::move(rr_scheduler));

  std::vector<std::shared_ptr<Process>> processes = {
      std::make_shared<Process>(1, "P1", 0, 10),
      std::make_shared<Process>(2, "P2", 1, 8),
      std::make_shared<Process>(3, "P3", 2, 6),
      std::make_shared<Process>(4, "P4", 3, 4)};

  scheduler.load_processes(processes);
  scheduler.run_until_completion();

  print_results(scheduler);
}

void demo_priority() {
  print_header("Priority Scheduling (Lower number = Higher priority)");

  CPUScheduler scheduler;
  scheduler.set_scheduler(std::make_unique<PriorityScheduler>());

  std::vector<std::shared_ptr<Process>> processes = {
      std::make_shared<Process>(1, "P1", 0, 8, 3),
      std::make_shared<Process>(2, "P2", 1, 4, 1),
      std::make_shared<Process>(3, "P3", 2, 9, 4),
      std::make_shared<Process>(4, "P4", 3, 5, 2)};

  scheduler.load_processes(processes);
  scheduler.run_until_completion();

  print_results(scheduler);
}

void demo_from_file(const std::string &process_file,
                    const std::string &config_file) {
  print_header("Simulación desde archivo");

  try {
    auto config = ConfigParser::load_simulator_config(config_file);
    auto processes = ConfigParser::load_processes_from_file(process_file);

    if (processes.empty()) {
      std::cerr << "No se cargaron procesos." << std::endl;
      return;
    }

    std::cout << "\nConfiguración del simulador:\n";
    std::cout << "  Marcos de memoria: " << config.total_memory_frames << "\n";
    std::cout << "  Tamaño de marco: " << config.frame_size << " bytes\n";
    std::cout << "  Algoritmo de planificación: " << config.scheduling_algorithm
              << "\n";
    std::cout << "  Algoritmo de reemplazo: "
              << config.page_replacement_algorithm << "\n";
    std::cout << "  Quantum: " << config.quantum << "\n\n";

    CPUScheduler scheduler;

    if (config.scheduling_algorithm == "FCFS") {
      scheduler.set_scheduler(std::make_unique<FCFSScheduler>());
    } else if (config.scheduling_algorithm == "SJF") {
      scheduler.set_scheduler(std::make_unique<SJFScheduler>());
    } else if (config.scheduling_algorithm == "RoundRobin") {
      scheduler.set_scheduler(
          std::make_unique<RoundRobinScheduler>(config.quantum));
    } else if (config.scheduling_algorithm == "Priority") {
      scheduler.set_scheduler(std::make_unique<PriorityScheduler>());
    } else {
      std::cerr << "Algoritmo de planificación no reconocido: "
                << config.scheduling_algorithm << std::endl;
      return;
    }

    scheduler.load_processes(processes);
    scheduler.run_until_completion();

    print_results(scheduler);

  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
  }
}

void print_usage(const char *program_name) {
  std::cout << "Uso: " << program_name << " [opciones]\n\n";
  std::cout << "Opciones:\n";
  std::cout << "  -f <archivo_procesos>  Cargar procesos desde archivo\n";
  std::cout << "  -c <archivo_config>    Cargar configuración desde archivo\n";
  std::cout << "  -h, --help             Mostrar esta ayuda\n\n";
  std::cout << "Si no se especifican archivos, se ejecuta la demostración por "
               "defecto.\n\n";
  std::cout << "Ejemplo:\n";
  std::cout << "  " << program_name
            << " -f data/procesos/procesos.txt -c data/procesos/config.txt\n";
}

int main(int argc, char *argv[]) {
  std::string process_file;
  std::string config_file;

  for (int i = 1; i < argc; i++) {
    if (std::strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
      process_file = argv[++i];
    } else if (std::strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
      config_file = argv[++i];
    } else if (std::strcmp(argv[i], "-h") == 0 ||
               std::strcmp(argv[i], "--help") == 0) {
      print_usage(argv[0]);
      return 0;
    }
  }

  std::cout << "\n";
  std::cout << "====================================================\n";
  std::cout << "|   CPU Scheduling Algorithms Demonstration        |\n";
  std::cout << "|   Operating System Simulator                     |\n";
  std::cout << "====================================================\n";

  if (!process_file.empty() && !config_file.empty()) {
    demo_from_file(process_file, config_file);
  } else if (!process_file.empty() || !config_file.empty()) {
    std::cerr << "\nError: Se deben especificar ambos archivos (-f y -c) o "
                 "ninguno.\n";
    print_usage(argv[0]);
    return 1;
  } else {
    demo_fcfs();
    demo_sjf();
    demo_round_robin();
    demo_priority();
  }

  std::cout << "\n========================================\n";
  std::cout << "  Demonstration Complete\n";
  std::cout << "========================================\n\n";

  return 0;
}
