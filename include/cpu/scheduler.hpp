#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include "core/process.hpp"
#include <memory>

namespace OSSimulator {

enum class SchedulingAlgorithm {
  FCFS,        //!< First Come, First Served
  SJF,         //!< Shortest Job First
  ROUND_ROBIN, //!< Round Robin
  PRIORITY     //!< Priority
};

/**
 * Clase base abstracta para los algoritmos de planificación de CPU.
 */
class Scheduler {
public:
  virtual ~Scheduler() = default;

  /**
   * Agrega un proceso a la cola de listos.
   *
   * @param process Proceso a agregar.
   */
  virtual void add_process(std::shared_ptr<Process> process) = 0;

  /**
   * Obtiene el siguiente proceso a ejecutar.
   *
   * @return Puntero al siguiente proceso a ejecutar.
   */
  virtual std::shared_ptr<Process> get_next_process() = 0;

  /**
   * Verifica si hay procesos en la cola de listos.
   *
   * @return true si hay procesos en la cola, false en caso contrario.
   */
  virtual bool has_processes() const = 0;

  /**
   * Elimina un proceso de la cola de listos por su PID.
   *
   * @param pid Identificador del proceso a eliminar.
   */
  virtual void remove_process(int pid) = 0;

  /**
   * Obtiene el número de procesos en la cola de listos.
   *
   * @return Número de procesos en la cola.
   */
  virtual size_t size() const = 0;

  /**
   * Limpia la cola de listos.
   */
  virtual void clear() = 0;

  /**
   * Obtiene el algoritmo de planificación utilizado.
   *
   * @return Algoritmo de planificación.
   */
  virtual SchedulingAlgorithm get_algorithm() const = 0;
};

} // namespace OSSimulator

#endif // SCHEDULER_HPP
