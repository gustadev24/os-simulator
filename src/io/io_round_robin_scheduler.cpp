#include "io/io_round_robin_scheduler.hpp"
#include <algorithm>

namespace OSSimulator {

IORoundRobinScheduler::IORoundRobinScheduler(int q) : quantum(q) {}

void IORoundRobinScheduler::add_request(std::shared_ptr<IORequest> request) {
  queue.push_back(request);
}

std::shared_ptr<IORequest> IORoundRobinScheduler::get_next_request() {
  if (queue.empty())
    return nullptr;
  auto request = queue.front();
  queue.pop_front();
  return request;
}

bool IORoundRobinScheduler::has_requests() const { return !queue.empty(); }

void IORoundRobinScheduler::remove_request(std::shared_ptr<IORequest> request) {
  auto it = std::find(queue.begin(), queue.end(), request);
  if (it != queue.end()) {
    queue.erase(it);
  }
}

size_t IORoundRobinScheduler::size() const { return queue.size(); }

void IORoundRobinScheduler::clear() { queue.clear(); }

IOSchedulingAlgorithm IORoundRobinScheduler::get_algorithm() const {
  return IOSchedulingAlgorithm::ROUND_ROBIN;
}

int IORoundRobinScheduler::get_quantum() const { return quantum; }

void IORoundRobinScheduler::set_quantum(int q) { quantum = q; }

} // namespace OSSimulator
