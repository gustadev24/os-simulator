#include "core/config_parser.hpp"
#include "core/process.hpp"
#include "cpu/cpu_scheduler.hpp"
#include "cpu/fcfs_scheduler.hpp"
#include "cpu/priority_scheduler.hpp"
#include "cpu/round_robin_scheduler.hpp"
#include "cpu/sjf_scheduler.hpp"
#include "io/io_device.hpp"
#include "io/io_fcfs_scheduler.hpp"
#include "io/io_manager.hpp"
#include "memory/fifo_replacement.hpp"
#include "memory/lru_replacement.hpp"
#include "memory/memory_manager.hpp"
#include "metrics/metrics_collector.hpp"
#include <cstring>
#include <filesystem>
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

void demo_fcfs(std::shared_ptr<MetricsCollector> metrics = nullptr) {
  print_header("FCFS (First Come First Served)");

  CPUScheduler scheduler;
  scheduler.set_scheduler(std::make_unique<FCFSScheduler>());
  
  if (metrics) {
    scheduler.set_metrics_collector(metrics);
  }

  std::vector<std::shared_ptr<Process>> processes = {
      std::make_shared<Process>(1, "P1", 0, 8),
      std::make_shared<Process>(2, "P2", 1, 4),
      std::make_shared<Process>(3, "P3", 2, 9),
      std::make_shared<Process>(4, "P4", 3, 5)};

  scheduler.load_processes(processes);
  scheduler.run_until_completion();

  print_results(scheduler);
}

void demo_sjf(std::shared_ptr<MetricsCollector> metrics = nullptr) {
  print_header("SJF (Shortest Job First)");

  CPUScheduler scheduler;
  scheduler.set_scheduler(std::make_unique<SJFScheduler>());
  
  if (metrics) {
    scheduler.set_metrics_collector(metrics);
  }

  std::vector<std::shared_ptr<Process>> processes = {
      std::make_shared<Process>(1, "P1", 0, 8),
      std::make_shared<Process>(2, "P2", 1, 4),
      std::make_shared<Process>(3, "P3", 2, 2),
      std::make_shared<Process>(4, "P4", 3, 1)};

  scheduler.load_processes(processes);
  scheduler.run_until_completion();

  print_results(scheduler);
}

void demo_round_robin(std::shared_ptr<MetricsCollector> metrics = nullptr) {
  print_header("Round Robin (Quantum = 4)");

  CPUScheduler scheduler;
  auto rr_scheduler = std::make_unique<RoundRobinScheduler>(4);
  scheduler.set_scheduler(std::move(rr_scheduler));
  
  if (metrics) {
    scheduler.set_metrics_collector(metrics);
  }

  std::vector<std::shared_ptr<Process>> processes = {
      std::make_shared<Process>(1, "P1", 0, 10),
      std::make_shared<Process>(2, "P2", 1, 8),
      std::make_shared<Process>(3, "P3", 2, 6),
      std::make_shared<Process>(4, "P4", 3, 4)};

  scheduler.load_processes(processes);
  scheduler.run_until_completion();

  print_results(scheduler);
}

void demo_priority(std::shared_ptr<MetricsCollector> metrics = nullptr) {
  print_header("Priority Scheduling (Lower number = Higher priority)");

  CPUScheduler scheduler;
  scheduler.set_scheduler(std::make_unique<PriorityScheduler>());
  
  if (metrics) {
    scheduler.set_metrics_collector(metrics);
  }

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
                    const std::string &config_file,
                    std::shared_ptr<MetricsCollector> metrics = nullptr) {
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
    
    // Setup memory manager
    std::unique_ptr<ReplacementAlgorithm> replacement_algo;
    if (config.page_replacement_algorithm == "FIFO") {
      replacement_algo = std::make_unique<FIFOReplacement>();
    } else if (config.page_replacement_algorithm == "LRU") {
      replacement_algo = std::make_unique<LRUReplacement>();
    } else {
      // Default to FIFO if not recognized
      replacement_algo = std::make_unique<FIFOReplacement>();
    }
    
    auto memory_manager = std::make_shared<MemoryManager>(
        config.total_memory_frames, std::move(replacement_algo), 1);
    
    // Setup I/O manager
    auto io_manager = std::make_shared<IOManager>();
    
    // Add default disk device with FCFS scheduler
    auto disk_device = std::make_shared<IODevice>("disk");
    disk_device->set_scheduler(std::make_unique<IOFCFSScheduler>());
    io_manager->add_device("disk", disk_device);
    
    // Connect managers to scheduler
    scheduler.set_memory_manager(memory_manager);
    scheduler.set_io_manager(io_manager);
    
    // Connect metrics if enabled
    if (metrics) {
      scheduler.set_metrics_collector(metrics);
      memory_manager->set_metrics_collector(metrics);
      io_manager->set_metrics_collector(metrics);
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
  std::cout << "  -f <archivo_procesos>  Archivo de procesos (default: data/procesos/procesos.txt)\n";
  std::cout << "  -c <archivo_config>    Archivo de configuración (default: data/procesos/config.txt)\n";
  std::cout << "  -m [archivo_metricas]  Habilitar métricas (default: data/resultados/metrics.jsonl)\n";
  std::cout << "  -d, --demo             Ejecutar demostración con algoritmos predefinidos\n";
  std::cout << "  -h, --help             Mostrar esta ayuda\n\n";
  std::cout << "Comportamiento por defecto:\n";
  std::cout << "  Sin opciones: Carga procesos y configuración desde archivos por defecto\n";
  std::cout << "  con métricas habilitadas.\n\n";
  std::cout << "Ejemplos:\n";
  std::cout << "  " << program_name << "                    # Usa archivos y métricas por defecto\n";
  std::cout << "  " << program_name << " --demo             # Ejecuta demos de algoritmos\n";
  std::cout << "  " << program_name << " -f custom.txt -c config.txt\n";
  std::cout << "  " << program_name << " -m custom_metrics.jsonl\n";
}

int main(int argc, char *argv[]) {
  // Default file paths
  std::string process_file = "data/procesos/procesos.txt";
  std::string config_file = "data/procesos/config.txt";
  std::string metrics_file = "data/resultados/metrics.jsonl";
  bool run_demo = false;
  bool enable_metrics = true;

  for (int i = 1; i < argc; i++) {
    if (std::strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
      process_file = argv[++i];
    } else if (std::strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
      config_file = argv[++i];
    } else if (std::strcmp(argv[i], "-m") == 0) {
      enable_metrics = true;
      if (i + 1 < argc && argv[i + 1][0] != '-') {
        metrics_file = argv[++i];
      }
    } else if (std::strcmp(argv[i], "-d") == 0 ||
               std::strcmp(argv[i], "--demo") == 0) {
      run_demo = true;
    } else if (std::strcmp(argv[i], "-h") == 0 ||
               std::strcmp(argv[i], "--help") == 0) {
      print_usage(argv[0]);
      return 0;
    }
  }

  // Create metrics collector if enabled
  std::shared_ptr<MetricsCollector> metrics;
  if (enable_metrics) {
    // Ensure the file path uses data/resultados/ directory
    std::string final_metrics_path;
    if (metrics_file.find('/') == std::string::npos) {
      // No directory specified, use data/resultados/
      final_metrics_path = "data/resultados/" + metrics_file;
    } else {
      // User specified a path, use it as-is
      final_metrics_path = metrics_file;
    }
    
    // Create directory if it doesn't exist
    std::filesystem::path metrics_path(final_metrics_path);
    std::filesystem::path metrics_dir = metrics_path.parent_path();
    
    if (!metrics_dir.empty()) {
      std::filesystem::create_directories(metrics_dir);
      
      // Delete existing metrics file if it exists
      if (std::filesystem::exists(final_metrics_path)) {
        std::filesystem::remove(final_metrics_path);
        std::cout << "[INFO] Eliminado archivo antiguo: \"" << final_metrics_path << "\"\n";
      }
    }
    
    metrics = std::make_shared<MetricsCollector>();
    if (metrics->enable_file_output(final_metrics_path)) {
      std::cout << "[INFO] Métricas habilitadas. Salida: " << final_metrics_path << "\n";
      metrics_file = final_metrics_path; // Update for later message
    } else {
      std::cerr << "\n[ERROR] No se pudo abrir el archivo de métricas: " 
                << final_metrics_path << "\n";
      return 1;
    }
  }

  std::cout << "\n";
  std::cout << "====================================================\n";
  std::cout << "|   CPU Scheduling Algorithms Demonstration        |\n";
  std::cout << "|   Operating System Simulator                     |\n";
  std::cout << "====================================================\n";

  if (run_demo) {
    // Run demo mode with predefined algorithms
    demo_fcfs(metrics);
    demo_sjf(metrics);
    demo_round_robin(metrics);
    demo_priority(metrics);
  } else {
    // Run simulation from files
    demo_from_file(process_file, config_file, metrics);
  }

  // Flush metrics if enabled
  if (metrics) {
    metrics->flush_all();
    metrics->disable_output();
    std::cout << "\n[INFO] Métricas guardadas en: " << metrics_file << "\n";
  }

  std::cout << "\n========================================\n";
  std::cout << "  Demonstration Complete\n";
  std::cout << "========================================\n\n";

  return 0;
}
