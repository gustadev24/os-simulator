#ifndef IO_SCHEDULER_HPP
#define IO_SCHEDULER_HPP

#include "io/io_request.hpp"
#include <memory>

namespace OSSimulator {

enum class IOSchedulingAlgorithm {
  FCFS,        //!< First Come, First Served
  SJF,         //!< Shortest Job First
  ROUND_ROBIN, //!< Round Robin
  PRIORITY     //!< Priority
};

/**
 * Clase base abstracta para los algoritmos de planificación de E/S.
 */
class IOScheduler {
public:
  virtual ~IOScheduler() = default;

  /**
   * Agrega una solicitud de E/S a la cola.
   *
   * @param request Solicitud de E/S a agregar.
   */
  virtual void add_request(std::shared_ptr<IORequest> request) = 0;

  /**
   * Obtiene la siguiente solicitud de E/S a ejecutar.
   *
   * @return Puntero a la siguiente solicitud de E/S a ejecutar.
   */
  virtual std::shared_ptr<IORequest> get_next_request() = 0;

  /**
   * Verifica si hay solicitudes de E/S en la cola.
   *
   * @return true si hay solicitudes en la cola, false en caso contrario.
   */
  virtual bool has_requests() const = 0;

  /**
   * Elimina una solicitud de E/S de la cola.
   *
   * @param request Solicitud de E/S a eliminar.
   */
  virtual void remove_request(std::shared_ptr<IORequest> request) = 0;

  /**
   * Obtiene el número de solicitudes de E/S en la cola.
   *
   * @return Número de solicitudes en la cola.
   */
  virtual size_t size() const = 0;

  /**
   * Limpia la cola de solicitudes de E/S.
   */
  virtual void clear() = 0;

  /**
   * Obtiene el algoritmo de planificación de E/S utilizado.
   *
   * @return Algoritmo de planificación de E/S.
   */
  virtual IOSchedulingAlgorithm get_algorithm() const = 0;
};

} // namespace OSSimulator

#endif // IO_SCHEDULER_HPP
