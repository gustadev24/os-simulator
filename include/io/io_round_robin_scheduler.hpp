#ifndef IO_ROUND_ROBIN_SCHEDULER_HPP
#define IO_ROUND_ROBIN_SCHEDULER_HPP

#include "io/io_scheduler.hpp"
#include <deque>
#include <memory>

namespace OSSimulator {

/**
 * Clase que implementa el algoritmo de planificación Round Robin para E/S.
 */
class IORoundRobinScheduler : public IOScheduler {
private:
  std::deque<std::shared_ptr<IORequest>> queue; //!< Cola de solicitudes de E/S.
  int quantum; //!< Quantum de tiempo para cada solicitud.

public:
  /**
   * Constructor por defecto.
   *
   * @param q Quantum de tiempo (por defecto 4).
   */
  explicit IORoundRobinScheduler(int q = 4);

  void add_request(std::shared_ptr<IORequest> request) override;
  std::shared_ptr<IORequest> get_next_request() override;
  bool has_requests() const override;
  void remove_request(std::shared_ptr<IORequest> request) override;
  size_t size() const override;
  void clear() override;
  IOSchedulingAlgorithm get_algorithm() const override;

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

#endif // IO_ROUND_ROBIN_SCHEDULER_HPP
