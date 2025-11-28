#include "core/config_parser.hpp"
#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include <memory>

using namespace OSSimulator;

TEST_CASE("ConfigParser parse burst sequence", "[config_parser]") {
  SECTION("Parse single CPU burst") {
    std::string burst_str = "CPU(10)";
    auto bursts = ConfigParser::parse_burst_sequence(burst_str);

    REQUIRE(bursts.size() == 1);
    REQUIRE(bursts[0].type == BurstType::CPU);
    REQUIRE(bursts[0].duration == 10);
  }

  SECTION("Parse CPU and IO bursts") {
    std::string burst_str = "CPU(4),E/S(3),CPU(5)";
    auto bursts = ConfigParser::parse_burst_sequence(burst_str);

    REQUIRE(bursts.size() == 3);
    REQUIRE(bursts[0].type == BurstType::CPU);
    REQUIRE(bursts[0].duration == 4);
    REQUIRE(bursts[1].type == BurstType::IO);
    REQUIRE(bursts[1].duration == 3);
    REQUIRE(bursts[2].type == BurstType::CPU);
    REQUIRE(bursts[2].duration == 5);
  }

  SECTION("Parse complex burst sequence") {
    std::string burst_str = "CPU(5),E/S(4),CPU(3),E/S(2),CPU(4)";
    auto bursts = ConfigParser::parse_burst_sequence(burst_str);

    REQUIRE(bursts.size() == 5);
    REQUIRE(bursts[0].duration == 5);
    REQUIRE(bursts[1].duration == 4);
    REQUIRE(bursts[2].duration == 3);
    REQUIRE(bursts[3].duration == 2);
    REQUIRE(bursts[4].duration == 4);
  }
}

TEST_CASE("ConfigParser parse process line", "[config_parser]") {
  SECTION("Parse valid process line") {
    std::string line = "P1 0 CPU(4),E/S(3),CPU(5) 1 4";
    auto process = ConfigParser::parse_process_line(line);

    REQUIRE(process != nullptr);
    REQUIRE(process->pid == 1);
    REQUIRE(process->name == "P1");
    REQUIRE(process->arrival_time == 0);
    REQUIRE(process->priority == 1);
    REQUIRE(process->memory_required == 4);
    REQUIRE(process->burst_sequence.size() == 3);
  }

  SECTION("Parse process without priority and pages") {
    std::string line = "P2 2 CPU(6)";
    auto process = ConfigParser::parse_process_line(line);

    REQUIRE(process != nullptr);
    REQUIRE(process->pid == 2);
    REQUIRE(process->arrival_time == 2);
    REQUIRE(process->priority == 0);
    REQUIRE(process->memory_required == 0);
  }

  SECTION("Parse process with only CPU burst") {
    std::string line = "P3 4 CPU(8) 3 6";
    auto process = ConfigParser::parse_process_line(line);

    REQUIRE(process != nullptr);
    REQUIRE(process->burst_sequence.size() == 1);
    REQUIRE(process->burst_sequence[0].type == BurstType::CPU);
    REQUIRE(process->burst_sequence[0].duration == 8);
  }

  SECTION("Skip comment line") {
    std::string line = "# This is a comment";
    auto process = ConfigParser::parse_process_line(line);

    REQUIRE(process == nullptr);
  }

  SECTION("Skip empty line") {
    std::string line = "   ";
    auto process = ConfigParser::parse_process_line(line);

    REQUIRE(process == nullptr);
  }
}

TEST_CASE("ConfigParser load processes from file", "[config_parser]") {
  SECTION("Load valid process file") {
    std::string temp_file = "test_procesos.txt";
    std::ofstream out(temp_file);
    out << "# Test process file\n";
    out << "P1 0 CPU(4),E/S(3),CPU(5) 1 4\n";
    out << "P2 2 CPU(6),E/S(2),CPU(3) 2 5\n";
    out << "P3 4 CPU(8) 3 6\n";
    out.close();

    auto processes = ConfigParser::load_processes_from_file(temp_file);

    REQUIRE(processes.size() == 3);
    REQUIRE(processes[0]->pid == 1);
    REQUIRE(processes[1]->pid == 2);
    REQUIRE(processes[2]->pid == 3);

    std::remove(temp_file.c_str());
  }

  SECTION("Throw exception for non-existent file") {
    REQUIRE_THROWS_AS(
        ConfigParser::load_processes_from_file("non_existent.txt"),
        std::runtime_error);
  }
}

TEST_CASE("ConfigParser load simulator config", "[config_parser]") {
  SECTION("Load valid config file") {
    std::string temp_file = "test_config.txt";
    std::ofstream out(temp_file);
    out << "# Simulator configuration\n";
    out << "total_memory_frames=64\n";
    out << "frame_size=4096\n";
    out << "scheduling_algorithm=RoundRobin\n";
    out << "page_replacement_algorithm=LRU\n";
    out << "quantum=4\n";
    out.close();

    auto config = ConfigParser::load_simulator_config(temp_file);

    REQUIRE(config.total_memory_frames == 64);
    REQUIRE(config.frame_size == 4096);
    REQUIRE(config.scheduling_algorithm == "RoundRobin");
    REQUIRE(config.page_replacement_algorithm == "LRU");
    REQUIRE(config.quantum == 4);

    std::remove(temp_file.c_str());
  }

  SECTION("Load config with default values") {
    std::string temp_file = "test_config_empty.txt";
    std::ofstream out(temp_file);
    out << "# Empty config\n";
    out.close();

    auto config = ConfigParser::load_simulator_config(temp_file);

    REQUIRE(config.total_memory_frames == 0);
    REQUIRE(config.frame_size == 4096);
    REQUIRE(config.quantum == 4);

    std::remove(temp_file.c_str());
  }

  SECTION("Throw exception for non-existent config file") {
    REQUIRE_THROWS_AS(
        ConfigParser::load_simulator_config("non_existent_config.txt"),
        std::runtime_error);
  }
}
