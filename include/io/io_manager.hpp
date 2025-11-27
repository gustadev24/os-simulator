#ifndef IO_MANAGER_HPP
#define IO_MANAGER_HPP

#include "io/io_device.hpp"
#include "metrics/metrics_collector.hpp"
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace OSSimulator {

/**
 * Clase que gestiona múltiples dispositivos de entrada/salida.
 */
class IOManager {
private:
  std::map<std::string, std::shared_ptr<IODevice>>
      devices;                      //!< Mapa de dispositivos de E/S.
  mutable std::mutex manager_mutex; //!< Mutex para sincronización.

  using CompletionCallback = std::function<void(std::shared_ptr<Process>, int)>;
  CompletionCallback
      completion_callback; //!< Callback al completar una solicitud.

  std::shared_ptr<MetricsCollector> metrics_collector;

public:
  IOManager() = default;
  ~IOManager() = default;

  /**
   * Agrega un dispositivo de E/S al gestor.
   *
   * @param name Nombre del dispositivo.
   * @param device Puntero al dispositivo de E/S.
   */
  void add_device(const std::string &name, std::shared_ptr<IODevice> device);

  /**
   * Obtiene un dispositivo de E/S por su nombre.
   *
   * @param name Nombre del dispositivo.
   * @return Puntero al dispositivo de E/S.
   */
  std::shared_ptr<IODevice> get_device(const std::string &name);

  /**
   * Verifica si existe un dispositivo con el nombre especificado.
   *
   * @param name Nombre del dispositivo.
   * @return true si el dispositivo existe, false en caso contrario.
   */
  bool has_device(const std::string &name) const;

  /**
   * Establece el callback de finalización para todos los dispositivos.
   *
   * @param callback Función a llamar cuando se complete una solicitud.
   */
  void set_completion_callback(CompletionCallback callback);

  /**
   * Envía una solicitud de E/S al dispositivo correspondiente.
   *
   * @param request Solicitud de E/S a enviar.
   */
  void submit_io_request(std::shared_ptr<IORequest> request);

  /**
   * Ejecuta un paso de simulación en todos los dispositivos.
   *
   * @param quantum Quantum de tiempo a ejecutar.
   * @param current_time Tiempo actual del sistema.
   */
  void execute_all_devices(int quantum, int current_time);

  void set_metrics_collector(std::shared_ptr<MetricsCollector> collector);
  /**
   * Verifica si hay operaciones de E/S pendientes en algún dispositivo.
   *
   * @return true si hay operaciones pendientes, false en caso contrario.
   */
  bool has_pending_io() const;

  /**
   * Reinicia las estadísticas de todos los dispositivos.
   */
  void reset_all_devices();

  /**
   * Obtiene todos los dispositivos gestionados.
   *
   * @return Mapa con todos los dispositivos.
   */
  std::map<std::string, std::shared_ptr<IODevice>> get_all_devices() const {
    std::lock_guard<std::mutex> lock(manager_mutex);
    return devices;
  }

  /**
   * Genera una salida JSON estructurada con el estado del planificador de E/S.
   *
   * @return String JSON con las estadísticas y estado de todos los dispositivos.
   */
  std::string generate_json_output() const;

  /**
   * Guarda el estado del planificador de E/S en un archivo JSON.
   *
   * @param filename Nombre del archivo donde guardar el JSON.
   * @return true si se guardó correctamente, false en caso contrario.
   */
  bool save_json_to_file(const std::string &filename) const;
};

} // namespace OSSimulator

#endif // IO_MANAGER_HPP
