#include "core/config_parser.hpp"
#include "cpu/cpu_scheduler.hpp"
#include "cpu/fcfs_scheduler.hpp"
#include "cpu/priority_scheduler.hpp"
#include "cpu/round_robin_scheduler.hpp"
#include "cpu/sjf_scheduler.hpp"
#include "io/io_device.hpp"
#include "io/io_fcfs_scheduler.hpp"
#include "io/io_manager.hpp"
#include "io/io_round_robin_scheduler.hpp"
#include "memory/fifo_replacement.hpp"
#include "memory/lru_replacement.hpp"
#include "memory/memory_manager.hpp"
#include "memory/nru_replacement.hpp"
#include "memory/optimal_replacement.hpp"
#include "metrics/metrics_collector.hpp"
#include <cstring>
#include <filesystem>
#include <iostream>
#include <memory>
#include <vector>

using namespace OSSimulator;

/**
 * Ejecuta la simulación con los archivos de configuración y procesos especificados.
 * @param process_file Ruta al archivo de definición de procesos.
 * @param config_file Ruta al archivo de configuración del simulador.
 * @param metrics Colector de métricas opcional para registrar la ejecución.
 */
void run_simulation(const std::string &process_file,
                    const std::string &config_file,
                    std::shared_ptr<MetricsCollector> metrics = nullptr) {
  try {
    auto config = ConfigParser::load_simulator_config(config_file);
    auto processes = ConfigParser::load_processes_from_file(process_file);

    if (processes.empty()) {
      std::cerr << "[ERROR] No se cargaron procesos." << std::endl;
      return;
    }

    std::cout << "\n[CONFIGURACIÓN]\n";
    std::cout << "  Archivo de procesos:      " << process_file << "\n";
    std::cout << "  Archivo de configuración: " << config_file << "\n";
    std::cout << "  Marcos de memoria:        " << config.total_memory_frames
              << "\n";
    std::cout << "  Tamaño de marco:          " << config.frame_size
              << " bytes\n";
    std::cout << "  Algoritmo de CPU:         " << config.scheduling_algorithm
              << "\n";
    std::cout << "  Algoritmo de reemplazo:   "
              << config.page_replacement_algorithm << "\n";
    std::cout << "  Algoritmo de E/S:         "
              << config.io_scheduling_algorithm << "\n";
    std::cout << "  Quantum:                  " << config.quantum << "\n";
    std::cout << "  Quantum E/S:              " << config.io_quantum << "\n";
    std::cout << "  Procesos cargados:        " << processes.size() << "\n";

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
      std::cerr << "[ERROR] Algoritmo de planificación no reconocido: "
                << config.scheduling_algorithm << std::endl;
      return;
    }

    std::unique_ptr<ReplacementAlgorithm> replacement_algo;
    if (config.page_replacement_algorithm == "FIFO") {
      replacement_algo = std::make_unique<FIFOReplacement>();
    } else if (config.page_replacement_algorithm == "LRU") {
      replacement_algo = std::make_unique<LRUReplacement>();
    } else if (config.page_replacement_algorithm == "Optimal") {
      replacement_algo = std::make_unique<OptimalReplacement>();
    } else if (config.page_replacement_algorithm == "NRU") {
      replacement_algo = std::make_unique<NRUReplacement>();
    } else {
      replacement_algo = std::make_unique<FIFOReplacement>();
    }

    auto memory_manager = std::make_shared<MemoryManager>(
        config.total_memory_frames, std::move(replacement_algo), 1);

    auto io_manager = std::make_shared<IOManager>();
    auto disk_device = std::make_shared<IODevice>("disk");

    if (config.io_scheduling_algorithm == "RoundRobin") {
      disk_device->set_scheduler(
          std::make_unique<IORoundRobinScheduler>(config.io_quantum));
    } else {
      disk_device->set_scheduler(std::make_unique<IOFCFSScheduler>());
    }
    io_manager->add_device("disk", disk_device);

    scheduler.set_memory_manager(memory_manager);
    scheduler.set_io_manager(io_manager);

    if (metrics) {
      scheduler.set_metrics_collector(metrics);
      memory_manager->set_metrics_collector(metrics);
      io_manager->set_metrics_collector(metrics);
    }

    scheduler.load_processes(processes);
    scheduler.run_until_completion();

  } catch (const std::exception &e) {
    std::cerr << "[ERROR] " << e.what() << std::endl;
  }
}

/**
 * Muestra el mensaje de ayuda con las opciones disponibles.
 * @param program_name Nombre del ejecutable.
 */
void print_usage(const char *program_name) {
  std::cout << "NOMBRE\n";
  std::cout << "    os_simulator - Simulador de planificación de procesos y "
               "memoria virtual\n\n";
  std::cout << "SINOPSIS\n";
  std::cout << "    " << program_name << " [OPCIONES]\n\n";
  std::cout << "DESCRIPCIÓN\n";
  std::cout << "    Simula la ejecución de procesos con diferentes algoritmos "
               "de planificación\n";
  std::cout << "    de CPU y gestión de memoria virtual. Genera métricas para "
               "análisis posterior.\n\n";
  std::cout << "OPCIONES\n";
  std::cout << "    -f <archivo>\n";
  std::cout << "        Especifica el archivo de procesos a cargar.\n";
  std::cout << "        Por defecto: data/procesos/procesos.txt\n\n";
  std::cout << "    -c <archivo>\n";
  std::cout
      << "        Especifica el archivo de configuración del simulador.\n";
  std::cout << "        Por defecto: data/procesos/config.txt\n\n";
  std::cout << "    -m <archivo>\n";
  std::cout
      << "        Especifica el archivo donde se guardarán las métricas.\n";
  std::cout << "        Por defecto: data/resultados/metrics.jsonl\n\n";
  std::cout << "    -h, --help\n";
  std::cout << "        Muestra esta ayuda.\n\n";
  std::cout << "EJEMPLOS\n";
  std::cout << "    # Ejecutar con configuración por defecto\n";
  std::cout << "    " << program_name << "\n\n";
  std::cout << "    # Usar archivos personalizados\n";
  std::cout << "    " << program_name
            << " -f mis_procesos.txt -c mi_config.txt\n\n";
  std::cout << "    # Especificar archivo de métricas personalizado\n";
  std::cout << "    " << program_name << " -m resultados/test.jsonl\n";
}

/**
 * Muestra las instrucciones para generar diagramas de visualización.
 * @param metrics_file Ruta al archivo de métricas generado.
 */
void print_visualization_instructions(const std::string &metrics_file) {
  std::cout << "\n[DIAGRAMAS]\n";
  std::cout << "  Para visualizar los resultados, ejecute:\n";
  std::cout << "    python -m visualization " << metrics_file << "\n";
  std::cout << "  Los diagramas se guardarán en: data/diagramas/\n";
}

/**
 * Punto de entrada principal del simulador.
 * @param argc Número de argumentos de línea de comandos.
 * @param argv Arreglo de argumentos de línea de comandos.
 * @return Código de salida (0 = éxito, 1 = error).
 */
int main(int argc, char *argv[]) {
  std::string process_file = "data/procesos/procesos.txt";
  std::string config_file = "data/procesos/config.txt";
  std::string metrics_file = "data/resultados/metrics.jsonl";
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
    } else if (std::strcmp(argv[i], "-h") == 0 ||
               std::strcmp(argv[i], "--help") == 0) {
      print_usage(argv[0]);
      return 0;
    }
  }

  std::shared_ptr<MetricsCollector> metrics;
  if (enable_metrics) {
    std::string final_metrics_path;
    if (metrics_file.find('/') == std::string::npos) {
      final_metrics_path = "data/resultados/" + metrics_file;
    } else {
      final_metrics_path = metrics_file;
    }

    std::filesystem::path metrics_path(final_metrics_path);
    std::filesystem::path metrics_dir = metrics_path.parent_path();

    if (!metrics_dir.empty()) {
      std::filesystem::create_directories(metrics_dir);
      if (std::filesystem::exists(final_metrics_path)) {
        std::filesystem::remove(final_metrics_path);
      }
    }

    metrics = std::make_shared<MetricsCollector>();
    if (!metrics->enable_file_output(final_metrics_path)) {
      std::cerr << "[ERROR] No se pudo abrir el archivo de métricas: "
                << final_metrics_path << "\n";
      return 1;
    }
    metrics_file = final_metrics_path;
  }

  run_simulation(process_file, config_file, metrics);

  if (metrics) {
    metrics->flush_all();
    metrics->disable_output();
    std::cout << "\n[INFO] Métricas guardadas en: " << metrics_file << "\n";
    print_visualization_instructions(metrics_file);
  }

  std::cout << "\n[INFO] Simulación completada.\n\n";

  return 0;
}
