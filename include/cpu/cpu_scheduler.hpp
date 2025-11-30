#ifndef CPU_SCHEDULER_HPP
#define CPU_SCHEDULER_HPP

#include "core/process.hpp"
#include "cpu/scheduler.hpp"
#include "memory/memory_manager.hpp"
#include "metrics/metrics_collector.hpp"
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace OSSimulator {

class IOManager;

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

  std::shared_ptr<MemoryManager> memory_manager; //!< Gestor de memoria.
  std::shared_ptr<IOManager> io_manager;         //!< Gestor de E/S.
  std::shared_ptr<MetricsCollector>
      metrics_collector; //!< Recolector de métricas.
  bool pending_preemption =
      false;              //!< Indica si se debe preemptar el proceso actual.
  int total_cpu_time = 0; //!< Tiempo total de CPU utilizado.
  bool last_tick_was_idle = false; //!< Indica si el último tick fue idle.

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

  /**
    * Maneja la finalización de una operación de E/S.
    *
    * @param proc Proceso que completó la E/S.
    * @param completion_time Tiempo de finalización de la E/S.
    */
  void handle_io_completion(std::shared_ptr<Process> proc, int completion_time);

  /**
   * Avanza los dispositivos de E/S en el tiempo.
   *
   * @param time_slice Quantum de tiempo para avanzar.
   * @param step_start_time Tiempo de inicio del paso actual.
   * @param lock Mutex bloqueado para sincronización.
   */
  void advance_io_devices(int time_slice, int step_start_time,
                          std::unique_lock<std::mutex> &lock);

  /**
   * Maneja la preparación de la memoria para un proceso.
   *
   * @param proc Proceso que está listo para la memoria.
   */
  void handle_memory_ready(std::shared_ptr<Process> proc);

  /**
   * Avanza el gestor de memoria en el tiempo.
   *
   * @param time_slice Quantum de tiempo para avanzar.
   * @param step_start_time Tiempo de inicio del paso actual.
   * @param lock Mutex bloqueado para sincronización.
   */
  void advance_memory_manager(int time_slice, int step_start_time,
                              std::unique_lock<std::mutex> &lock);

  /**
   * Solicita la preempción si es necesario.
   *
   * @param proc Proceso que podría ser preemptado.
   */
  void request_preemption_if_needed(std::shared_ptr<Process> proc);

  /**
   * Verifica si se debe preemptar un proceso basado en la prioridad.
   *
   * @param candidate Proceso candidato para preempción.
   * @return true si se debe preemptar, false en caso contrario.
   */
  bool should_preempt_priority(std::shared_ptr<Process> candidate) const;

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
     * Establece el gestor de memoria a utilizar.
     *
     * @param mm Puntero al gestor de memoria.
     */
  void set_memory_manager(std::shared_ptr<MemoryManager> mm);

  /**
   * Establece el gestor de E/S a utilizar.
   *
   * @param manager Puntero al gestor de E/S.
   */
  void set_io_manager(std::shared_ptr<IOManager> manager);

  /**
   * Establece el recolector de métricas a utilizar.
   *
   * @param collector Puntero al recolector de métricas.
   */
  void set_metrics_collector(std::shared_ptr<MetricsCollector> collector);

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
   * Calcula la utilización de CPU.
   *
   * @return Porcentaje de utilización de CPU (0.0 - 100.0).
   */
  double get_cpu_utilization() const;

  /**
   * Obtiene el nombre del algoritmo de planificación actual.
   *
   * @return Nombre del algoritmo.
   */
  std::string get_algorithm_name() const;

  /**
   * Obtiene el número de procesos en la cola de listos.
   *
   * @return Tamaño de la cola de listos.
   */
  size_t get_ready_queue_size() const;

  /**
   * Obtiene los PIDs de procesos en estado READY.
   *
   * @return Vector con PIDs en cola de listos.
   */
  std::vector<int> get_ready_queue_pids() const;

  /**
   * Obtiene los PIDs de procesos en estado MEMORY_WAITING.
   *
   * @return Vector con PIDs bloqueados por memoria.
   */
  std::vector<int> get_memory_waiting_pids() const;

  /**
   * Obtiene los PIDs de procesos en estado WAITING (bloqueados por I/O).
   *
   * @return Vector con PIDs bloqueados por I/O.
   */
  std::vector<int> get_io_waiting_pids() const;

  /**
   * Obtiene el PID del proceso en ejecución.
   *
   * @return PID del proceso corriendo, o -1 si no hay ninguno.
   */
  int get_running_pid() const;

  /**
   * Reinicia el estado del planificador y de los procesos.
   */
  void reset();

private:
  /**
   * Envía las métricas del tick actual al recolector.
   *
   * @param event Evento a registrar.
   * @param proc Proceso involucrado (puede ser nullptr).
   * @param context_switch Si hubo cambio de contexto.
   */
  void send_cpu_metrics(const std::string &event, std::shared_ptr<Process> proc,
                        bool context_switch);

  /**
   * Envía un snapshot del estado de las colas al recolector.
   */
  void send_queue_snapshot();

  /**
   * Envía un snapshot del estado de las colas al recolector en un tick específico.
   */
  void send_queue_snapshot(int tick);
};

} // namespace OSSimulator

#endif // CPU_SCHEDULER_HPP
