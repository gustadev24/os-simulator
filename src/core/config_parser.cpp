#include "core/config_parser.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <stdexcept>

namespace OSSimulator {

std::string ConfigParser::trim(const std::string &str) {
  auto start = str.begin();
  while (start != str.end() && std::isspace(*start)) {
    start++;
  }

  auto end = str.end();
  do {
    end--;
  } while (std::distance(start, end) > 0 && std::isspace(*end));

  return std::string(start, end + 1);
}

bool ConfigParser::starts_with(const std::string &str,
                                const std::string &prefix) {
  return str.size() >= prefix.size() &&
         str.compare(0, prefix.size(), prefix) == 0;
}

std::vector<Burst> ConfigParser::parse_burst_sequence(const std::string &burst_str) {
  std::vector<Burst> bursts;
  std::regex burst_regex(R"((CPU|E/S)\((\d+)\))");
  std::sregex_iterator iter(burst_str.begin(), burst_str.end(), burst_regex);
  std::sregex_iterator end;

  while (iter != end) {
    std::smatch match = *iter;
    std::string type_str = match[1].str();
    int duration = std::stoi(match[2].str());

    BurstType type = (type_str == "CPU") ? BurstType::CPU : BurstType::IO;
    std::string device = (type == BurstType::IO) ? "default" : "";

    bursts.emplace_back(type, duration, device);
    ++iter;
  }

  return bursts;
}

std::shared_ptr<Process>
ConfigParser::parse_process_line(const std::string &line) {
  std::string trimmed_line = trim(line);

  if (trimmed_line.empty() || trimmed_line[0] == '#') {
    return nullptr;
  }

  std::istringstream iss(trimmed_line);
  std::string pid_str;
  int arrival_time;
  std::string burst_str;
  int priority = 0;
  int pages_required = 0;

  if (!(iss >> pid_str >> arrival_time >> burst_str)) {
    std::cerr << "Error parsing line: " << line << std::endl;
    return nullptr;
  }

  iss >> priority;
  iss >> pages_required;

  std::vector<Burst> bursts = parse_burst_sequence(burst_str);

  if (bursts.empty()) {
    std::cerr << "Error: no bursts found in line: " << line << std::endl;
    return nullptr;
  }

  int total_burst_time = 0;
  for (const auto &burst : bursts) {
    total_burst_time += burst.duration;
  }

  int pid = 0;
  try {
    if (starts_with(pid_str, "P")) {
      pid = std::stoi(pid_str.substr(1));
    } else {
      pid = std::stoi(pid_str);
    }
  } catch (const std::exception &e) {
    std::cerr << "Error parsing PID: " << pid_str << std::endl;
    return nullptr;
  }

  uint32_t memory_required =
      pages_required > 0 ? static_cast<uint32_t>(pages_required) : 0;

  auto process = std::make_shared<Process>(pid, pid_str, arrival_time, bursts,
                                           priority, memory_required);

  return process;
}

std::vector<std::shared_ptr<Process>>
ConfigParser::load_processes_from_file(const std::string &filename) {
  std::vector<std::shared_ptr<Process>> processes;
  std::ifstream file(filename);

  if (!file.is_open()) {
    throw std::runtime_error("No se pudo abrir el archivo: " + filename);
  }

  std::string line;
  int line_number = 0;
  while (std::getline(file, line)) {
    line_number++;
    auto process = parse_process_line(line);
    if (process) {
      processes.push_back(process);
    }
  }

  file.close();

  if (processes.empty()) {
    std::cerr << "Warning: No se cargaron procesos del archivo " << filename
              << std::endl;
  } else {
    std::cout << "Se cargaron " << processes.size() << " procesos de "
              << filename << std::endl;
  }

  return processes;
}

SimulatorConfig
ConfigParser::load_simulator_config(const std::string &filename) {
  SimulatorConfig config;
  std::ifstream file(filename);

  if (!file.is_open()) {
    throw std::runtime_error("No se pudo abrir el archivo de configuración: " +
                             filename);
  }

  std::string line;
  while (std::getline(file, line)) {
    std::string trimmed = trim(line);
    if (trimmed.empty() || trimmed[0] == '#') {
      continue;
    }

    std::istringstream iss(trimmed);
    std::string key, value;
    if (std::getline(iss, key, '=') && std::getline(iss, value)) {
      key = trim(key);
      value = trim(value);

      if (key == "total_memory_frames") {
        config.total_memory_frames = std::stoi(value);
      } else if (key == "frame_size") {
        config.frame_size = std::stoi(value);
      } else if (key == "scheduling_algorithm") {
        config.scheduling_algorithm = value;
      } else if (key == "page_replacement_algorithm") {
        config.page_replacement_algorithm = value;
      } else if (key == "quantum") {
        config.quantum = std::stoi(value);
      }
    }
  }

  file.close();
  return config;
}

} // namespace OSSimulator
