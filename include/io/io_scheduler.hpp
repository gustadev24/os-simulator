#ifndef IO_SCHEDULER_HPP
#define IO_SCHEDULER_HPP

#include "io/io_request.hpp"
#include <memory>

namespace OSSimulator {

enum class IOSchedulingAlgorithm { FCFS, SJF, ROUND_ROBIN, PRIORITY };

class IOScheduler {
public:
  virtual ~IOScheduler() = default;

  virtual void add_request(std::shared_ptr<IORequest> request) = 0;
  virtual std::shared_ptr<IORequest> get_next_request() = 0;
  virtual bool has_requests() const = 0;
  virtual void remove_request(std::shared_ptr<IORequest> request) = 0;
  virtual size_t size() const = 0;
  virtual void clear() = 0;
  virtual IOSchedulingAlgorithm get_algorithm() const = 0;
};

} // namespace OSSimulator

#endif // IO_SCHEDULER_HPP
