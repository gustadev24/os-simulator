#ifndef PROCESS_HPP
#define PROCESS_HPP

#include "core/burst.hpp"
#include "memory/page.hpp"
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace OSSimulator {

/**
 * Estados posibles de un proceso en la simulación.
 */
enum class ProcessState {
  NEW,            //!< Proceso recién creado.
  READY,          //!< Proceso listo para ejecutarse.
  MEMORY_WAITING, //!< Proceso bloqueado esperando carga de páginas.
  RUNNING,        //!< Proceso en ejecución.
  WAITING,        //!< Proceso en espera.
  TERMINATED      //!< Proceso terminado.
};

/**
 * Representa un proceso dentro de la simulación del sistema operativo.
 * Esta estructura contiene toda la información relevante sobre un proceso,
 * incluyendo sus tiempos de llegada, ráfaga, métricas de rendimiento, estado
 * actual, y componentes de threading para la simulación concurrente.
 *
 */
struct Process {
  int pid;             //!< Identificador único del proceso.
  std::string name;    //!< Nombre del proceso.
  int arrival_time;    //!< Tiempo de llegada del proceso.
  int burst_time;      //!< Duración total de la ráfaga del proceso.
  int remaining_time;  //!< Tiempo restante para completar la ráfaga.
  int completion_time; //!< Tiempo en que el proceso finaliza su ejecución.
  int waiting_time;    //!< Tiempo que el proceso ha estado esperando.
  int turnaround_time; //!< Tiempo total desde la llegada hasta la finalización.
  int response_time;   //!< Tiempo desde la llegada hasta la primera ejecución.
  int start_time;      //!< Tiempo en que el proceso comienza su ejecución.
  int priority;        //!< Prioridad del proceso.
  std::atomic<ProcessState> state; //!< Estado actual del proceso.
  bool
      first_execution; //!< Indica si el proceso ha sido ejecutado por primera vez.
  int last_execution_time;  //!< Último tiempo en que el proceso fue ejecutado.
  uint32_t memory_required; //!< Memoria requerida por el proceso.
  uint32_t memory_base;     //!< Dirección base de la memoria asignada.
  bool memory_allocated;    //!< Indica si la memoria ha sido asignada.

  std::vector<Page> page_table; //!< Tabla de páginas asignadas al proceso.
  std::vector<int>
      memory_access_trace; //!< Rastro de accesos a memoria (índices de página).
  size_t current_access_index =
      0; //!< Índice actual dentro del rastro de accesos.

  int page_faults = 0;        //!< Contador de fallos de página del proceso.
  int replacements = 0;       //!< Contador de reemplazos de páginas realizados.
  int active_pages_count = 0; //!< Número de páginas activas actualmente.

  std::vector<Burst>
      burst_sequence;         //!< Secuencia de ráfagas (CPU/E/S) del proceso.
  size_t current_burst_index; //!< Índice de la ráfaga actual.
  int total_cpu_time;         //!< Tiempo total consumido por CPU.
  int total_io_time;          //!< Tiempo total consumido en E/S.
  std::unique_ptr<std::thread> process_thread; //!< Hilo asociado al proceso.
  mutable std::mutex process_mutex; //!< Mutex para sincronización del proceso.
  mutable std::condition_variable
      state_cv; //!< Variable de condición para notificar cambios de estado.
  std::atomic<bool> should_terminate; //!< Indica si el hilo debe terminar.
  std::atomic<bool>
      step_complete; //!< Indica si el paso de ejecución está completo.

  /**
   * Obtiene el siguiente acceso a página según el rastro de accesos.
   * @return Índice de página del siguiente acceso, o -1 si no hay más accesos.
   */
  int get_next_page_access() const {
    if (current_access_index < memory_access_trace.size()) {
      return memory_access_trace[current_access_index];
    }
    return -1;
  }

  /**
   * Avanza el índice del rastro de accesos de memoria.
   */
  void advance_page_access() {
    if (current_access_index < memory_access_trace.size()) {
      current_access_index++;
    }
  }

  /**
   * Constructor por defecto.
   */
  Process();

  /**
   * Constructor parametrizado.
   *
   * @param p ID del proceso.
   * @param n Nombre del proceso.
   * @param arrival Tiempo de llegada.
   * @param burst Tiempo de ráfaga.
   * @param prio Prioridad del proceso.
   * @param mem Memoria requerida por el proceso.
   */
  Process(int p, const std::string &n, int arrival, int burst, int prio = 0,
          uint32_t mem = 0);

  /**
   * Constructor parametrizado con secuencia de ráfagas.
   * @param p ID del proceso.
   * @param n Nombre del proceso.
   * @param arrival Tiempo de llegada.
   * @param bursts Secuencia de ráfagas del proceso.
   * @param prio Prioridad del proceso.
   * @param mem Memoria requerida por el proceso.
   */
  Process(int p, const std::string &n, int arrival,
          const std::vector<Burst> &bursts, int prio = 0, uint32_t mem = 0);

  Process(const Process &other) = delete;

  Process &operator=(const Process &other) = delete;

  /**
   * Constructor de movimiento.
   *
   * @param other Proceso a mover.
   */
  Process(Process &&other) noexcept;

  /**
   * Calcula y actualiza las métricas del proceso.
   */
  void calculate_metrics();

  /**
   * Verifica si el proceso ha llegado al tiempo actual.
   *
   * @param current_time Tiempo actual de la simulación.
   * @return true si el proceso ha llegado, false en caso contrario.
   */
  bool has_arrived(int current_time) const;

  /**
   * Verifica si el proceso ha completado su ejecución.
   *
   * @return true si el proceso está completo, false en caso contrario.
   */
  bool is_completed() const;

  /**
   * Ejecuta el proceso por un quantum de tiempo dado.
   *
   * @param quantum Quantum de tiempo para ejecutar el proceso.
   * @param current_time Tiempo actual de la simulación.
   * @return Tiempo realmente ejecutado.
   */
  int execute(int quantum, int current_time);

  /**
   * Reinicia el estado del proceso.
   */
  void reset();

  /**
   * Verifica si el proceso tiene más ráfagas por ejecutar.
   *
   * @return true si hay más ráfagas, false en caso contrario.
   */
  bool has_more_bursts() const;

  /**
   * Obtiene la ráfaga actual del proceso.
   *
   * @return Puntero constante a la ráfaga actual.
   */
  const Burst *get_current_burst() const;

  /**
   * Obtiene la ráfaga actual del proceso (mutable).
   *
   * @return Puntero mutable a la ráfaga actual.
   */
  Burst *get_current_burst_mutable();

  /**
   * Avanza a la siguiente ráfaga en la secuencia.
   */
  void advance_to_next_burst();

  /**
   * Verifica si el proceso está en una ráfaga de CPU.
   *
   * @return true si está en ráfaga de CPU, false en caso contrario.
   */
  bool is_on_cpu_burst() const;

  /**
   * Verifica si el proceso está en una ráfaga de E/S.
   *
   * @return true si está en ráfaga de E/S, false en caso contrario.
   */
  bool is_on_io_burst() const;

  /**
   * Obtiene el tiempo total de ráfaga del proceso.
   *
   * @return Tiempo total de ráfaga.
   */
  int get_total_burst_time() const;

  /**
   * Destructor del proceso.
   * Detiene el hilo si está en ejecución.
   */
  ~Process();

  /**
   * Inicia el hilo del proceso.
   */
  void start_thread();

  /**
   * Detiene el hilo del proceso.
   */
  void stop_thread();

  /**
   * Verifica si el hilo del proceso está en ejecución.
   *
   * @return true si el hilo está en ejecución, false en caso contrario.
   */
  bool is_thread_running() const;

private:
  /**
   * Función principal del hilo del proceso.
   */
  void thread_function();
};

} // namespace OSSimulator

#endif // PROCESS_HPP
