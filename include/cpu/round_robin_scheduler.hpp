#ifndef ROUND_ROBIN_SCHEDULER_HPP
#define ROUND_ROBIN_SCHEDULER_HPP

#include "cpu/scheduler.hpp"
#include <deque>

namespace OSSimulator {

/**
 * Clase que implementa el algoritmo de planificación Round Robin.
 */
class RoundRobinScheduler : public Scheduler {
private:
  std::deque<std::shared_ptr<Process>>
      ready_queue; //!< Cola de procesos listos.
  int quantum;     //!< Quantum de tiempo para cada proceso.

public:
  /**
   * Constructor por defecto.
   *
   * @param q Quantum de tiempo (por defecto 4).
   */
  explicit RoundRobinScheduler(int q = 4);

  void add_process(std::shared_ptr<Process> process) override;
  std::shared_ptr<Process> get_next_process() override;
  bool has_processes() const override;
  void remove_process(int pid) override;
  size_t size() const override;
  void clear() override;
  SchedulingAlgorithm get_algorithm() const override;

  /**
   * Rota la cola de procesos listos.
   */
  void rotate();

  /**
   * Obtiene el quantum de tiempo.
   *
   * @return Quantum de tiempo.
   */
  int get_quantum() const;

  /**
   * Establece el quantum de tiempo.
   *
   * @param q Nuevo quantum de tiempo.
   */
  void set_quantum(int q);
};

} // namespace OSSimulator

#endif // ROUND_ROBIN_SCHEDULER_HPP
