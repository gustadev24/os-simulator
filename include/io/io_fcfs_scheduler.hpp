#ifndef IO_FCFS_SCHEDULER_HPP
#define IO_FCFS_SCHEDULER_HPP

#include "io/io_scheduler.hpp"
#include <deque>
#include <memory>

namespace OSSimulator {

/**
 * Clase que implementa el algoritmo de planificación First Come, First Served (FCFS) para E/S.
 */
class IOFCFSScheduler : public IOScheduler {
private:
  std::deque<std::shared_ptr<IORequest>> queue; //!< Cola de solicitudes de E/S.

public:
  void add_request(std::shared_ptr<IORequest> request) override;
  std::shared_ptr<IORequest> get_next_request() override;
  bool has_requests() const override;
  void remove_request(std::shared_ptr<IORequest> request) override;
  size_t size() const override;
  void clear() override;
  IOSchedulingAlgorithm get_algorithm() const override;
};

} // namespace OSSimulator

#endif // IO_FCFS_SCHEDULER_HPP
