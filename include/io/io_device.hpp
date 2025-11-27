#ifndef IO_DEVICE_HPP
#define IO_DEVICE_HPP

#include "io/io_request.hpp"
#include "io/io_scheduler.hpp"
#include "metrics/metrics_collector.hpp"
#include <functional>
#include <memory>
#include <mutex>
#include <string>

namespace OSSimulator {

/**
 * Clase que representa un dispositivo de entrada/salida.
 */
class IODevice {
private:
  std::string device_name;                //!< Nombre del dispositivo.
  std::unique_ptr<IOScheduler> scheduler; //!< Planificador de E/S.
  std::shared_ptr<IORequest>
      current_request; //!< Solicitud actualmente en ejecución.

  int total_io_time;   //!< Tiempo total de E/S ejecutado.
  int device_switches; //!< Número de cambios de contexto del dispositivo.
  int total_requests_completed; //!< Total de solicitudes completadas.

  mutable std::mutex device_mutex; //!< Mutex para sincronización.

  using CompletionCallback = std::function<void(std::shared_ptr<Process>, int)>;
  CompletionCallback
      completion_callback; //!< Callback al completar una solicitud.

  std::shared_ptr<MetricsCollector> metrics_collector;
  bool last_event_was_completed;

public:
  /**
   * Constructor.
   *
   * @param name Nombre del dispositivo.
   */
  explicit IODevice(const std::string &name);
  ~IODevice() = default;

  /**
   * Establece el planificador del dispositivo.
   *
   * @param sched Planificador de E/S a utilizar.
   */
  void set_scheduler(std::unique_ptr<IOScheduler> sched);

  /**
   * Establece el callback de finalización.
   *
   * @param callback Función a llamar cuando se complete una solicitud.
   */
  void set_completion_callback(CompletionCallback callback);
  void set_metrics_collector(std::shared_ptr<MetricsCollector> collector);

  /**
   * Agrega una solicitud de E/S a la cola del dispositivo.
   *
   * @param request Solicitud de E/S a agregar.
   */
  void add_io_request(std::shared_ptr<IORequest> request);

  /**
   * Ejecuta un paso de simulación del dispositivo.
   *
   * @param quantum Quantum de tiempo a ejecutar.
   * @param current_time Tiempo actual del sistema.
   */
  void execute_step(int quantum, int current_time);

  /**
   * Verifica si hay solicitudes pendientes en la cola.
   *
   * @return true si hay solicitudes pendientes, false en caso contrario.
   */
  bool has_pending_requests() const;

  /**
   * Verifica si el dispositivo está ocupado ejecutando una solicitud.
   *
   * @return true si está ocupado, false en caso contrario.
   */
  bool is_busy() const;

  /**
   * Obtiene el nombre del dispositivo.
   *
   * @return Nombre del dispositivo.
   */
  std::string get_device_name() const { return device_name; }

  /**
   * Obtiene el tiempo total de E/S ejecutado.
   *
   * @return Tiempo total de E/S.
   */
  int get_total_io_time() const { return total_io_time; }

  /**
   * Obtiene el número de cambios de contexto del dispositivo.
   *
   * @return Número de cambios de contexto.
   */
  int get_device_switches() const { return device_switches; }

  /**
   * Obtiene el total de solicitudes completadas.
   *
   * @return Total de solicitudes completadas.
   */
  int get_total_requests_completed() const { return total_requests_completed; }

  /**
   * Obtiene el tamaño de la cola de solicitudes.
   *
   * @return Tamaño de la cola.
   */
  size_t get_queue_size() const;

  void send_log_metrics(int current_time);
  /**
   * Reinicia las estadísticas del dispositivo.
   */
  void reset();

  /**
   * Genera una salida JSON con el estado actual del dispositivo.
   *
   * @return String JSON con el estado y estadísticas del dispositivo.
   */
  std::string get_device_status_json() const;
};

} // namespace OSSimulator

#endif // IO_DEVICE_HPP
