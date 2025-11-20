#ifndef IO_ROUND_ROBIN_SCHEDULER_HPP
#define IO_ROUND_ROBIN_SCHEDULER_HPP

#include "io/io_scheduler.hpp"
#include <deque>
#include <memory>

namespace OSSimulator {

class IORoundRobinScheduler : public IOScheduler {
private:
  std::deque<std::shared_ptr<IORequest>> queue;
  int quantum;

public:
  explicit IORoundRobinScheduler(int q = 4);

  void add_request(std::shared_ptr<IORequest> request) override;
  std::shared_ptr<IORequest> get_next_request() override;
  bool has_requests() const override;
  void remove_request(std::shared_ptr<IORequest> request) override;
  size_t size() const override;
  void clear() override;
  IOSchedulingAlgorithm get_algorithm() const override;

  int get_quantum() const;
  void set_quantum(int q);
};

} // namespace OSSimulator

#endif // IO_ROUND_ROBIN_SCHEDULER_HPP
