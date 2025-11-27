#ifndef MEMORY_MANAGER_HPP
#define MEMORY_MANAGER_HPP

#include "memory/replacement_algorithm.hpp"
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace OSSimulator {

struct Process;

/**
 * Clase que gestiona la memoria en la simulación.
 */
class MemoryManager {
public:
  using ProcessReadyCallback = std::function<void(std::shared_ptr<Process>)>;

  /**
   * Constructor parametrizado.
   *
   * @param total_frames Número total de marcos físicos.
   * @param algo Algoritmo de reemplazo a utilizar.
   * @param page_fault_latency Latencia (en ticks) para cargar una página.
   */
  MemoryManager(int total_frames, std::unique_ptr<ReplacementAlgorithm> algo,
                int page_fault_latency = 1);

  /**
   * Registra un proceso para gestión de memoria.
   *
   * @param process Proceso a registrar.
   */
  void register_process(std::shared_ptr<Process> process);

  /**
   * Elimina el registro de un proceso.
    *
    * @param pid ID del proceso a desregistrar.
   */
  void unregister_process(int pid);

  /**
   * Establece el callback que se llamará cuando un proceso quede listo tras cargar páginas.
   *
    * @param callback Función a invocar.
   */
  void set_ready_callback(ProcessReadyCallback callback);

  /**
   * Intenta asignar memoria inicial al proceso.
    *
    * @param process Proceso al que asignar memoria.
   * @return true si la asignación inicial fue posible.
   */
  bool allocate_initial_memory(Process &process);

  /**
   * Prepara un proceso para ser ejecutado en CPU.
   *
    * @param process Proceso a preparar.
   * @param current_time Tiempo actual de la simulación.
   * @return true si el proceso está listo para CPU, false si hay faltas de página pendientes.
   */
  bool prepare_process_for_cpu(std::shared_ptr<Process> process,
                               int current_time);

  /**
   * Avanza la cola de fallos de página en el tiempo.
   *
    * @param duration Duración del avance en ticks.
    * @param start_time Tiempo de inicio para el avance.
   */
  void advance_fault_queue(int duration, int start_time);

  /**
   * Marca un proceso como inactivo respecto a la memoria.
    *
      * @param process Proceso a marcar como inactivo.
   */
  void mark_process_inactive(const Process &process);

  /**
   * Libera la memoria asociada a un proceso.
    *
    * @param pid ID del proceso cuya memoria se liberará.
   */
  void release_process_memory(int pid);

  /**
   * Obtiene el número total de fallos de página ocurridos.
   *
    * @return Número total de fallos de página.
   */
  int get_total_page_faults() const;

  /**
   * Obtiene el número total de reemplazos de páginas realizados.
    *
      * @return Número total de reemplazos.
   */
  int get_total_replacements() const;

  /**
   * Genera una salida JSON estructurada con el estado de la memoria.
   *
   * @return String JSON con las tablas de páginas, estado de marcos y estadísticas.
   */
  std::string generate_json_output();

  /**
   * Guarda el estado de la memoria en un archivo JSON.
   *
   * @param filename Nombre del archivo donde guardar el JSON.
   * @return true si se guardó correctamente, false en caso contrario.
   */
  bool save_json_to_file(const std::string &filename);

private:
  int total_frames; //!< Número de marcos físicos.
  std::unique_ptr<ReplacementAlgorithm>
      algorithm;          //!< Algoritmo de reemplazo usado.
  int page_fault_latency; //!< Latencia para cargar páginas.

  std::vector<Frame> frames; //!< Marcos físicos.
  std::unordered_map<int, std::shared_ptr<Process>>
      process_map;   //!< Procesos registrados.
  std::mutex mutex_; //!< Mutex para operaciones internas.

  struct PageLoadTask {
    std::shared_ptr<Process> process; //!< Proceso al que pertenece la página.
    int page_id;                      //!< ID de la página a cargar.
    int remaining_time; //!< Tiempo restante para completar la carga.
    int frame_id;       //!< Marco destino (si reservado).
    int enqueue_time;   //!< Tiempo en que se encoló la tarea.
  };

  std::deque<PageLoadTask>
      fault_queue; //!< Cola de cargas pendientes por fallo de página.
  std::optional<PageLoadTask>
      active_task; //!< Tarea de carga activa (si existe).
  std::unordered_map<int, std::unordered_set<int>>
      pending_pages_by_process; //!< Páginas pendientes por proceso.
  std::unordered_set<int>
      processes_waiting_on_memory; //!< PIDs esperando memoria.
  ProcessReadyCallback
      ready_callback;  //!< Callback a invocar cuando un proceso quede listo.
  int memory_time = 0; //!< Reloj interno para la gestión de memoria.

  int total_page_faults = 0;  //!< Contador total de fallos de página.
  int total_replacements = 0; //!< Contador total de reemplazos.

  /**
   * Busca un marco físico libre.
   *
   * @return Índice del marco libre, o -1 si no hay ninguno.
   */
  int find_free_frame();

  /**   
   * Verifica si todas las páginas de un proceso están residentes en memoria.
   * 
   * @param process Proceso a verificar.
   * @return true si todas las páginas están en memoria, false en caso contrario.
   */
  bool are_all_pages_resident(const Process &process) const;

  /**
    * Encola las páginas faltantes de un proceso para carga.
    * 
    * @param process Proceso al que pertenecen las páginas.
    * @param missing_pages Vector con los IDs de las páginas faltantes.
    * @param current_time Tiempo actual para registrar el encolado.
    */
  void enqueue_missing_pages(std::shared_ptr<Process> process,
                             const std::vector<int> &missing_pages,
                             int current_time);

  /**   
   * Inicia la siguiente tarea de carga si es posible.
   * 
   * @param current_time Tiempo actual para iniciar la tarea.
   */
  void start_next_task_if_possible(int current_time);

  /**   
   * Reserva un marco para la tarea de carga dada.
   * 
   * @param task Tarea de carga para la cual se reserva el marco.
   * @return true si se pudo reservar un marco, false en caso contrario.
   */
  bool reserve_frame_for_task(PageLoadTask &task);

  /**   
   * Completa la tarea de carga activa y actualiza el estado del proceso.
   * 
   * @param completion_time Tiempo en que se completa la carga.
   * @return Puntero al proceso que completó la carga.
   */
  std::shared_ptr<Process> complete_active_task(int completion_time);

  /**   
   * Libera un marco físico y actualiza el estado correspondiente.
   * 
   * @param frame_idx Índice del marco a liberar.
   */
  void evict_frame(int frame_idx);

  /**   
   * Marca todas las páginas de un proceso como referenciadas o no referenciadas.
   * 
   * @param process Proceso cuyas páginas se van a marcar.
   * @param referenced Valor para establecer en el bit de referencia.
   */
  void set_process_pages_referenced(const Process &process, bool referenced);
};

} // namespace OSSimulator

#endif
