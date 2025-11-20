#include "io/io_fcfs_scheduler.hpp"
#include <algorithm>

namespace OSSimulator {

void IOFCFSScheduler::add_request(std::shared_ptr<IORequest> request) {
  queue.push_back(request);
}

std::shared_ptr<IORequest> IOFCFSScheduler::get_next_request() {
  if (queue.empty())
    return nullptr;
  auto request = queue.front();
  queue.pop_front();
  return request;
}

bool IOFCFSScheduler::has_requests() const { return !queue.empty(); }

void IOFCFSScheduler::remove_request(std::shared_ptr<IORequest> request) {
  auto it = std::find(queue.begin(), queue.end(), request);
  if (it != queue.end()) {
    queue.erase(it);
  }
}

size_t IOFCFSScheduler::size() const { return queue.size(); }

void IOFCFSScheduler::clear() { queue.clear(); }

IOSchedulingAlgorithm IOFCFSScheduler::get_algorithm() const {
  return IOSchedulingAlgorithm::FCFS;
}

} // namespace OSSimulator
