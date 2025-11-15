#ifndef CPU_SCHEDULER_HPP
#define CPU_SCHEDULER_HPP

#include "core/process.hpp"
#include "cpu/scheduler.hpp"
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace OSSimulator {

/**
 * Clase que representa el planificador de CPU en la simulación.
 */
class CPUScheduler {
private:
  std::unique_ptr<Scheduler>
      scheduler; //!< Estrategia de planificación utilizada.

  std::vector<std::shared_ptr<Process>>
      all_processes; //!< Todos los procesos cargados.
  std::vector<std::shared_ptr<Process>>
      completed_processes; //!< Procesos completados.

  int current_time; //!< Tiempo actual de la simulación.
  std::shared_ptr<Process>
      running_process; //!< Proceso que se está ejecutando actualmente.

  int context_switches; //!< Número de cambios de contexto realizados.

  using MemoryCheckCallback = std::function<bool(
      const Process &)>; //!< Tipo de función para verificar la memoria.
  MemoryCheckCallback
      memory_check_callback; //!< Función para verificar la disponibilidad de memoria.

  std::mutex
      scheduler_mutex; //!< Mutex para proteger el acceso al planificador.
  std::atomic<bool>
      simulation_running; //!< Indica si la simulación está en ejecución.

  /**
   * Crea y lanza un hilo para el proceso dado.
   *
   * @param proc Proceso para el cual se crea el hilo.
   */
  void spawn_process_thread(std::shared_ptr<Process> proc);

  /**
   * Notifica que un proceso está en ejecución.
   *
   * @param proc Proceso que está en ejecución.
   */
  void notify_process_running(std::shared_ptr<Process> proc);

  /**
   * Espera a que un proceso complete su paso de ejecución.
   *
   * @param proc Proceso que está esperando completar su paso.
   */
  void wait_for_process_step(std::shared_ptr<Process> proc);

  /**
   * Termina todos los hilos de los procesos en ejecución.
   */
  void terminate_all_threads();

public:
  /**
   * Constructor por defecto.
   */
  CPUScheduler();

  /**
   * Destructor del planificador.
   * Detiene todos los hilos de los procesos en ejecución.
   */
  ~CPUScheduler();

  /**
   * Establece la estrategia de planificación a utilizar.
   *
   * @param sched Puntero a la estrategia de planificación.
   */
  void set_scheduler(std::unique_ptr<Scheduler> sched);

  /**
   * Establece la función de verificación de memoria.
   *
   * @param callback Función de verificación de memoria.
   */
  void set_memory_callback(MemoryCheckCallback callback);

  /**
   * Agrega un proceso al planificador y lanza su hilo.
   *
   * @param process Proceso a agregar.
   */
  void add_process(std::shared_ptr<Process> process);

  /**
   * Carga múltiples procesos en el planificador.
   *
   * @param processes Vector de procesos a cargar.
   */
  void load_processes(const std::vector<std::shared_ptr<Process>> &processes);

  /**
   * Verifica y asigna memoria para un proceso.
   *
   * @param process Proceso para el cual se verifica y asigna memoria.
   * @return true si la memoria fue asignada exitosamente, false en caso contrario.
   */
  bool check_and_allocate_memory(Process &process);

  /**
   * Agrega los procesos que han llegado al planificador.
   */
  void add_arrived_processes();

  /**
   * Ejecuta un paso de la simulación con un quantum dado.
   *
   * @param quantum Quantum de tiempo para el paso de ejecución.
   */
  void execute_step(int quantum = 1);

  /**
   * Ejecuta la simulación hasta que todos los procesos hayan completado.
   */
  void run_until_completion();

  /**
   * Verifica si hay procesos pendientes por ejecutar.
   *
   * @return true si hay procesos pendientes, false en caso contrario.
   */
  bool has_pending_processes() const;

  /**
   * Obtiene el tiempo actual de la simulación.
   *
   * @return Tiempo actual de la simulación.
   */
  int get_current_time() const;

  /**
   * Obtiene el número de cambios de contexto realizados.
   *
   * @return Número de cambios de contexto realizados.
   */
  int get_context_switches() const;

  /**
   * Obtiene los procesos completados.
   *
   * @return Vector de procesos completados.
   */
  const std::vector<std::shared_ptr<Process>> &get_completed_processes() const;

  /**
   * Obtiene todos los procesos cargados.
   *
   * @return Vector de todos los procesos cargados.
   */
  const std::vector<std::shared_ptr<Process>> &get_all_processes() const;

  /**
   * Calcula el tiempo promedio de espera de los procesos completados.
   *
   * @return Tiempo promedio de espera.
   */
  double get_average_waiting_time() const;

  /**
   * Calcula el tiempo promedio de retorno de los procesos completados.
   *
   * @return Tiempo promedio de retorno.
   */
  double get_average_turnaround_time() const;

  /**
   * Calcula el tiempo promedio de respuesta de los procesos completados.
   *
   * @return Tiempo promedio de respuesta.
   */
  double get_average_response_time() const;

  /**
   * Reinicia el estado del planificador y de los procesos.
   */
  void reset();
};

} // namespace OSSimulator

#endif // CPU_SCHEDULER_HPP
