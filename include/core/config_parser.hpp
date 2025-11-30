#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include "core/burst.hpp"
#include "core/process.hpp"
#include <memory>
#include <string>
#include <vector>

namespace OSSimulator {

/**
 * Parámetros de configuración del simulador.
 */
struct SimulatorConfig {
  int total_memory_frames = 0;
  int frame_size = 4096;
  std::string scheduling_algorithm;
  std::string page_replacement_algorithm;
  int quantum = 4;
};

/**
 * Parser de archivos de configuración del simulador.
 * Soporta la carga de procesos desde archivos de texto y configuración del
 * simulador.
 */
class ConfigParser {
public:
  /**
   * Carga procesos desde un archivo de texto.
   * Formato esperado: PID tiempo_llegada rafagas prioridad paginas_requeridas
   * Ejemplo: P1 0 CPU(4),E/S(3),CPU(5) 1 4
   *
   * @param filename Ruta del archivo de procesos.
   * @return Vector de procesos cargados.
   */
  static std::vector<std::shared_ptr<Process>>
  load_processes_from_file(const std::string &filename);

  /**
   * Carga la configuración del simulador desde un archivo.
   * @param filename Ruta del archivo de configuración.
   * @return Estructura de configuración del simulador.
   */
  static SimulatorConfig
  load_simulator_config(const std::string &filename);

  /**
   * Parsea una línea de proceso individual.
   * @param line Línea de texto con información del proceso.
   * @return Proceso creado o nullptr si la línea es inválida.
   */
  static std::shared_ptr<Process> parse_process_line(const std::string &line);

  /**
   * Parsea la secuencia de ráfagas de un proceso.
   * Formato: CPU(4),E/S(3),CPU(5)
   * @param burst_str Cadena con la secuencia de ráfagas.
   * @return Vector de ráfagas parseadas.
   */
  static std::vector<Burst> parse_burst_sequence(const std::string &burst_str);

private:
  static std::string trim(const std::string &str);
  static bool starts_with(const std::string &str, const std::string &prefix);
};

} // namespace OSSimulator

#endif // CONFIG_PARSER_HPP
