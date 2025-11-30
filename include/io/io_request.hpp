#ifndef IO_REQUEST_HPP
#define IO_REQUEST_HPP

#include "core/burst.hpp"
#include "core/process.hpp"
#include <memory>

namespace OSSimulator {

/**
 * Estructura que representa una solicitud de operación de E/S.
 */
struct IORequest {
  std::shared_ptr<Process>
      process;         //!< Proceso que realiza la solicitud de E/S.
  Burst burst;         //!< Ráfaga de E/S a ejecutar.
  int arrival_time;    //!< Tiempo de llegada de la solicitud.
  int completion_time; //!< Tiempo de finalización de la solicitud.
  int start_time;      //!< Tiempo de inicio de ejecución.
  int priority;        //!< Prioridad de la solicitud.

  /**
   * Constructor por defecto.
   */
  IORequest();

  /**
   * Constructor con parámetros.
   *
   * @param proc Proceso que realiza la solicitud.
   * @param b Ráfaga de E/S.
   * @param arrival Tiempo de llegada.
   * @param prio Prioridad de la solicitud (por defecto 0).
   */
  IORequest(std::shared_ptr<Process> proc, const Burst &b, int arrival,
            int prio = 0);

  /**
   * Verifica si la solicitud de E/S ha sido completada.
   *
   * @return true si la solicitud está completada, false en caso contrario.
   */
  bool is_completed() const;

  /**
   * Ejecuta la solicitud de E/S por un quantum de tiempo.
   *
   * @param quantum Tiempo máximo de ejecución.
   * @param current_time Tiempo actual del sistema.
   * @return Tiempo efectivamente ejecutado.
   */
  int execute(int quantum, int current_time);
};

} // namespace OSSimulator

#endif // IO_REQUEST_HPP
