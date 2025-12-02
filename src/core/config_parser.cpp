#include "core/config_parser.hpp"
#include <cctype>
#include <fstream>
#include <regex>
#include <sstream>
#include <stdexcept>

namespace OSSimulator {

/**
 * Elimina espacios en blanco al inicio y final de una cadena.
 * @param str Cadena a procesar.
 * @return Cadena sin espacios al inicio ni al final.
 */
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

/**
 * Verifica si una cadena comienza con un prefijo dado.
 * @param str Cadena a verificar.
 * @param prefix Prefijo a buscar.
 * @return true si la cadena comienza con el prefijo, false en caso contrario.
 */
bool ConfigParser::starts_with(const std::string &str,
                               const std::string &prefix) {
  return str.size() >= prefix.size() &&
         str.compare(0, prefix.size(), prefix) == 0;
}

/**
 * Parsea una secuencia de ráfagas desde una cadena.
 * @param burst_str Cadena con formato "CPU(x),E/S(y),CPU(z)".
 * @return Vector de ráfagas parseadas.
 */
std::vector<Burst>
ConfigParser::parse_burst_sequence(const std::string &burst_str) {
  std::vector<Burst> bursts;
  std::regex burst_regex(R"((CPU|E/S)\((\d+)\))");
  std::sregex_iterator iter(burst_str.begin(), burst_str.end(), burst_regex);
  std::sregex_iterator end;

  while (iter != end) {
    std::smatch match = *iter;
    std::string type_str = match[1].str();
    int duration = std::stoi(match[2].str());

    BurstType type = (type_str == "CPU") ? BurstType::CPU : BurstType::IO;
    std::string device = (type == BurstType::IO) ? "disk" : "";

    bursts.emplace_back(type, duration, device);
    ++iter;
  }

  return bursts;
}

/**
 * Parsea una línea del archivo de procesos.
 * @param line Línea a parsear con formato "PID tiempo_llegada ráfagas prioridad páginas".
 * @return Puntero al proceso creado, o nullptr si la línea es inválida o un comentario.
 */
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
    return nullptr;
  }

  iss >> priority;
  iss >> pages_required;

  std::vector<Burst> bursts = parse_burst_sequence(burst_str);

  if (bursts.empty()) {
    return nullptr;
  }

  int pid = 0;
  try {
    if (starts_with(pid_str, "P")) {
      pid = std::stoi(pid_str.substr(1));
    } else {
      pid = std::stoi(pid_str);
    }
  } catch (const std::exception &) {
    return nullptr;
  }

  uint32_t memory_required =
      pages_required > 0 ? static_cast<uint32_t>(pages_required) : 0;

  return std::make_shared<Process>(pid, pid_str, arrival_time, bursts, priority,
                                   memory_required);
}

/**
 * Carga procesos desde un archivo de texto.
 * @param filename Ruta al archivo de procesos.
 * @return Vector de procesos cargados.
 * @throws std::runtime_error Si no se puede abrir el archivo.
 */
std::vector<std::shared_ptr<Process>>
ConfigParser::load_processes_from_file(const std::string &filename) {
  std::vector<std::shared_ptr<Process>> processes;
  std::ifstream file(filename);

  if (!file.is_open()) {
    throw std::runtime_error("No se pudo abrir el archivo: " + filename);
  }

  std::string line;
  while (std::getline(file, line)) {
    auto process = parse_process_line(line);
    if (process) {
      processes.push_back(process);
    }
  }

  file.close();
  return processes;
}

/**
 * Carga la configuración del simulador desde un archivo.
 * @param filename Ruta al archivo de configuración.
 * @return Estructura con la configuración cargada.
 * @throws std::runtime_error Si no se puede abrir el archivo.
 */
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
      } else if (key == "io_scheduling_algorithm") {
        config.io_scheduling_algorithm = value;
      } else if (key == "io_quantum") {
        config.io_quantum = std::stoi(value);
      }
    }
  }

  file.close();
  return config;
}

} // namespace OSSimulator
