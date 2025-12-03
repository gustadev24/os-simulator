/**
 * @file test_algorithm_combinations.cpp
 * @brief Tests de integración para combinaciones clave de algoritmos.
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

bool run_simulation_combination(const std::string &process_file,
                                const std::string &cpu_algo,
                                const std::string &mem_algo,
                                const std::string &io_algo,
                                const std::string &output_file, int quantum = 4,
                                int frames = 64) {
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
      disk_device->set_scheduler(std::make_unique<IORoundRobinScheduler>(quantum));
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

// ============================================================================
// TESTS DE COMBINACIONES CLAVE
// ============================================================================

TEST_CASE("Combinaciones de algoritmos representativas",
          "[integration][combinations]") {

  const std::string process_file = "data/procesos/procesos_large.txt";
  const std::string output_dir = "data/resultados/combinations/";
  const int frames = 64;

  // Test representativos de cada scheduler con una combinación de memoria/IO
  SECTION("FCFS + LRU + IO FCFS") {
    std::string output = output_dir + "fcfs_lru_iofcfs.jsonl";
    REQUIRE(run_simulation_combination(process_file, "FCFS", "LRU", "FCFS",
                                       output, 4, frames));
  }

  SECTION("SJF + LRU + IO RoundRobin") {
    std::string output = output_dir + "sjf_lru_iorr.jsonl";
    REQUIRE(run_simulation_combination(process_file, "SJF", "LRU", "RoundRobin",
                                       output, 4, frames));
  }

  SECTION("RoundRobin + FIFO + IO RoundRobin") {
    std::string output = output_dir + "rr_fifo_iorr.jsonl";
    REQUIRE(run_simulation_combination(process_file, "RoundRobin", "FIFO",
                                       "RoundRobin", output, 6, frames));
  }

  SECTION("Priority + LRU + IO FCFS") {
    std::string output = output_dir + "priority_lru_iofcfs.jsonl";
    REQUIRE(run_simulation_combination("data/procesos/procesos_priority_test.txt",
                                       "Priority", "LRU", "FCFS", output, 4, frames));
  }
}

TEST_CASE("Tests con cargas de trabajo específicas",
          "[integration][workloads]") {

  const std::string output_dir = "data/resultados/workloads/";
  const int frames = 64;

  SECTION("Carga intensiva de CPU") {
    std::string output = output_dir + "cpu_heavy.jsonl";
    REQUIRE(run_simulation_combination("data/procesos/procesos_cpu_heavy.txt",
                                       "RoundRobin", "LRU", "FCFS", output, 10, frames));
  }

  SECTION("Carga intensiva de E/S") {
    std::string output = output_dir + "io_heavy.jsonl";
    REQUIRE(run_simulation_combination("data/procesos/procesos_io_heavy.txt",
                                       "FCFS", "LRU", "RoundRobin", output, 10, frames));
  }

  SECTION("Carga mixta") {
    std::string output = output_dir + "mixed.jsonl";
    REQUIRE(run_simulation_combination("data/procesos/procesos_mixed.txt",
                                       "SJF", "LRU", "FCFS", output, 10, frames));
  }
}
