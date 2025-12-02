/**
 * @file test_algorithm_combinations.cpp
 * @brief Tests de integración para todas las combinaciones de algoritmos.
 * 
 * Este archivo contiene tests que ejecutan el simulador con todas las
 * combinaciones posibles de algoritmos de CPU, memoria e E/S, generando
 * archivos de resultados para cada combinación.
 */

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
#include "metrics/metrics_collector.hpp"
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

using namespace OSSimulator;

namespace {

/**
 * @brief Ejecuta una simulación completa con la configuración especificada.
 * @param process_file Archivo de procesos.
 * @param cpu_algo Algoritmo de CPU.
 * @param mem_algo Algoritmo de reemplazo de páginas.
 * @param io_algo Algoritmo de E/S.
 * @param output_file Archivo de salida para métricas.
 * @param quantum Quantum para Round Robin de CPU.
 * @param frames Número de marcos de memoria disponibles.
 * @param io_quantum Quantum para Round Robin de E/S (por defecto usa el mismo que CPU).
 * @return true si la simulación se completó correctamente.
 */
bool run_simulation_combination(const std::string &process_file,
                                const std::string &cpu_algo,
                                const std::string &mem_algo,
                                const std::string &io_algo,
                                const std::string &output_file, int quantum = 4,
                                int frames = 64, int io_quantum = -1) {

  // Si io_quantum no se especifica, usar el mismo quantum que CPU
  if (io_quantum < 0) {
    io_quantum = quantum;
  }

  try {
    auto processes = ConfigParser::load_processes_from_file(process_file);
    if (processes.empty()) {
      return false;
    }

    CPUScheduler scheduler;

    if (cpu_algo == "FCFS") {
      scheduler.set_scheduler(std::make_unique<FCFSScheduler>());
    } else if (cpu_algo == "SJF") {
      scheduler.set_scheduler(std::make_unique<SJFScheduler>());
    } else if (cpu_algo == "RoundRobin") {
      scheduler.set_scheduler(std::make_unique<RoundRobinScheduler>(quantum));
    } else if (cpu_algo == "Priority") {
      scheduler.set_scheduler(std::make_unique<PriorityScheduler>());
    } else {
      return false;
    }

    std::unique_ptr<ReplacementAlgorithm> replacement_algo;
    if (mem_algo == "FIFO") {
      replacement_algo = std::make_unique<FIFOReplacement>();
    } else if (mem_algo == "LRU") {
      replacement_algo = std::make_unique<LRUReplacement>();
    } else {
      replacement_algo = std::make_unique<FIFOReplacement>();
    }

    auto memory_manager =
        std::make_shared<MemoryManager>(frames, std::move(replacement_algo), 1);

    auto io_manager = std::make_shared<IOManager>();
    auto disk_device = std::make_shared<IODevice>("disk");

    if (io_algo == "RoundRobin") {
      disk_device->set_scheduler(
          std::make_unique<IORoundRobinScheduler>(io_quantum));
    } else {
      disk_device->set_scheduler(std::make_unique<IOFCFSScheduler>());
    }
    io_manager->add_device("disk", disk_device);

    scheduler.set_memory_manager(memory_manager);
    scheduler.set_io_manager(io_manager);

    std::filesystem::path output_path(output_file);
    std::filesystem::create_directories(output_path.parent_path());

    if (std::filesystem::exists(output_file)) {
      std::filesystem::remove(output_file);
    }

    auto metrics = std::make_shared<MetricsCollector>();
    if (!metrics->enable_file_output(output_file)) {
      return false;
    }

    scheduler.set_metrics_collector(metrics);
    memory_manager->set_metrics_collector(metrics);
    io_manager->set_metrics_collector(metrics);

    scheduler.load_processes(processes);
    scheduler.run_until_completion();

    metrics->flush_all();
    metrics->disable_output();

    return std::filesystem::exists(output_file) &&
           std::filesystem::file_size(output_file) > 0;

  } catch (const std::exception &) {
    return false;
  }
}

} // namespace

TEST_CASE("Combinaciones FCFS con diferentes algoritmos de memoria",
          "[integration][combinations][fcfs]") {

  const std::string process_file = "data/procesos/procesos_large.txt";
  const std::string output_dir = "data/resultados/combinations/";
  const int frames = 64;

  SECTION("FCFS + FIFO + IO FCFS") {
    std::string output = output_dir + "fcfs_fifo_iofcfs.jsonl";
    REQUIRE(run_simulation_combination(process_file, "FCFS", "FIFO", "FCFS",
                                       output, 4, frames));
  }

  SECTION("FCFS + LRU + IO FCFS") {
    std::string output = output_dir + "fcfs_lru_iofcfs.jsonl";
    REQUIRE(run_simulation_combination(process_file, "FCFS", "LRU", "FCFS",
                                       output, 4, frames));
  }

  SECTION("FCFS + FIFO + IO RoundRobin") {
    std::string output = output_dir + "fcfs_fifo_iorr.jsonl";
    REQUIRE(run_simulation_combination(process_file, "FCFS", "FIFO",
                                       "RoundRobin", output, 4, frames));
  }

  SECTION("FCFS + LRU + IO RoundRobin") {
    std::string output = output_dir + "fcfs_lru_iorr.jsonl";
    REQUIRE(run_simulation_combination(process_file, "FCFS", "LRU",
                                       "RoundRobin", output, 4, frames));
  }
}

TEST_CASE("Combinaciones SJF con diferentes algoritmos de memoria",
          "[integration][combinations][sjf]") {

  const std::string process_file = "data/procesos/procesos_large.txt";
  const std::string output_dir = "data/resultados/combinations/";
  const int frames = 64;

  SECTION("SJF + FIFO + IO FCFS") {
    std::string output = output_dir + "sjf_fifo_iofcfs.jsonl";
    REQUIRE(run_simulation_combination(process_file, "SJF", "FIFO", "FCFS",
                                       output, 4, frames));
  }

  SECTION("SJF + LRU + IO FCFS") {
    std::string output = output_dir + "sjf_lru_iofcfs.jsonl";
    REQUIRE(run_simulation_combination(process_file, "SJF", "LRU", "FCFS",
                                       output, 4, frames));
  }

  SECTION("SJF + FIFO + IO RoundRobin") {
    std::string output = output_dir + "sjf_fifo_iorr.jsonl";
    REQUIRE(run_simulation_combination(process_file, "SJF", "FIFO",
                                       "RoundRobin", output, 4, frames));
  }

  SECTION("SJF + LRU + IO RoundRobin") {
    std::string output = output_dir + "sjf_lru_iorr.jsonl";
    REQUIRE(run_simulation_combination(process_file, "SJF", "LRU", "RoundRobin",
                                       output, 4, frames));
  }
}

TEST_CASE("Combinaciones RoundRobin con diferentes algoritmos de memoria",
          "[integration][combinations][rr]") {

  const std::string process_file = "data/procesos/procesos_large.txt";
  const std::string output_dir = "data/resultados/combinations/";
  const int frames = 64;
  const int quantum = 6;

  SECTION("RoundRobin + FIFO + IO FCFS") {
    std::string output = output_dir + "rr_fifo_iofcfs.jsonl";
    REQUIRE(run_simulation_combination(process_file, "RoundRobin", "FIFO",
                                       "FCFS", output, quantum, frames));
  }

  SECTION("RoundRobin + LRU + IO FCFS") {
    std::string output = output_dir + "rr_lru_iofcfs.jsonl";
    REQUIRE(run_simulation_combination(process_file, "RoundRobin", "LRU",
                                       "FCFS", output, quantum, frames));
  }

  SECTION("RoundRobin + FIFO + IO RoundRobin") {
    std::string output = output_dir + "rr_fifo_iorr.jsonl";
    REQUIRE(run_simulation_combination(process_file, "RoundRobin", "FIFO",
                                       "RoundRobin", output, quantum, frames));
  }

  SECTION("RoundRobin + LRU + IO RoundRobin") {
    std::string output = output_dir + "rr_lru_iorr.jsonl";
    REQUIRE(run_simulation_combination(process_file, "RoundRobin", "LRU",
                                       "RoundRobin", output, quantum, frames));
  }
}

TEST_CASE("Combinaciones Priority con diferentes algoritmos de memoria",
          "[integration][combinations][priority]") {

  const std::string process_file = "data/procesos/procesos_priority_test.txt";
  const std::string output_dir = "data/resultados/combinations/";
  const int frames = 64;

  SECTION("Priority + FIFO + IO FCFS") {
    std::string output = output_dir + "priority_fifo_iofcfs.jsonl";
    REQUIRE(run_simulation_combination(process_file, "Priority", "FIFO", "FCFS",
                                       output, 4, frames));
  }

  SECTION("Priority + LRU + IO FCFS") {
    std::string output = output_dir + "priority_lru_iofcfs.jsonl";
    REQUIRE(run_simulation_combination(process_file, "Priority", "LRU", "FCFS",
                                       output, 4, frames));
  }

  SECTION("Priority + FIFO + IO RoundRobin") {
    std::string output = output_dir + "priority_fifo_iorr.jsonl";
    REQUIRE(run_simulation_combination(process_file, "Priority", "FIFO",
                                       "RoundRobin", output, 4, frames));
  }

  SECTION("Priority + LRU + IO RoundRobin") {
    std::string output = output_dir + "priority_lru_iorr.jsonl";
    REQUIRE(run_simulation_combination(process_file, "Priority", "LRU",
                                       "RoundRobin", output, 4, frames));
  }
}

TEST_CASE("Tests con diferentes tipos de carga de trabajo",
          "[integration][workloads]") {

  const std::string output_dir = "data/resultados/workloads/";
  const int frames = 64;
  const int quantum = 10;

  SECTION("Carga intensiva de CPU con RoundRobin") {
    std::string output = output_dir + "cpu_heavy_rr.jsonl";
    REQUIRE(run_simulation_combination("data/procesos/procesos_cpu_heavy.txt",
                                       "RoundRobin", "LRU", "FCFS", output,
                                       quantum, frames));
  }

  SECTION("Carga intensiva de E/S con FCFS") {
    std::string output = output_dir + "io_heavy_fcfs.jsonl";
    REQUIRE(run_simulation_combination("data/procesos/procesos_io_heavy.txt",
                                       "FCFS", "FIFO", "FCFS", output,
                                       quantum, frames));
  }

  SECTION("Carga intensiva de E/S con RR") {
    std::string output = output_dir + "io_heavy_rr.jsonl";
    REQUIRE(run_simulation_combination("data/procesos/procesos_io_heavy.txt",
                                       "RoundRobin", "LRU", "RoundRobin",
                                       output, quantum, frames));
  }

  SECTION("Carga mixta con SJF") {
    std::string output = output_dir + "mixed_sjf.jsonl";
    REQUIRE(run_simulation_combination("data/procesos/procesos_mixed.txt",
                                       "SJF", "LRU", "FCFS", output, quantum,
                                       frames));
  }

  SECTION("Procesos extendidos con Priority") {
    std::string output = output_dir + "extended_priority.jsonl";
    REQUIRE(run_simulation_combination("data/procesos/procesos_extended.txt",
                                       "Priority", "LRU", "RoundRobin", output,
                                       quantum, frames));
  }
}

TEST_CASE("Tests de IO scheduling", "[integration][io_scheduling]") {

  const std::string process_file = "data/procesos/procesos_io_heavy.txt";
  const std::string output_dir = "data/resultados/io_tests/";
  const int frames = 64;
  const int quantum = 10;

  SECTION("IO FCFS con carga E/S intensiva") {
    std::string output = output_dir + "io_fcfs_heavy.jsonl";
    REQUIRE(run_simulation_combination(process_file, "FCFS", "LRU", "FCFS",
                                       output, quantum, frames));
  }

  SECTION("IO RoundRobin con carga E/S intensiva") {
    std::string output = output_dir + "io_rr_heavy.jsonl";
    REQUIRE(run_simulation_combination(process_file, "FCFS", "LRU",
                                       "RoundRobin", output, quantum, frames));
  }

  SECTION("Comparación IO FCFS vs RoundRobin - procesos mixtos") {
    std::string output_fcfs = output_dir + "io_comparison_fcfs.jsonl";
    std::string output_rr = output_dir + "io_comparison_rr.jsonl";

    REQUIRE(run_simulation_combination("data/procesos/procesos_mixed.txt",
                                       "RoundRobin", "LRU", "FCFS", output_fcfs,
                                       quantum, frames));

    REQUIRE(run_simulation_combination("data/procesos/procesos_mixed.txt",
                                       "RoundRobin", "LRU", "RoundRobin",
                                       output_rr, quantum, frames));
  }
}
