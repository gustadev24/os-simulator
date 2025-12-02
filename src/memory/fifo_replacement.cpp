#include "memory/fifo_replacement.hpp"
#include "core/process.hpp"
#include <algorithm>

namespace OSSimulator {

int FIFOReplacement::select_victim(
    const std::vector<Frame> &frames,
    const std::unordered_map<int, std::shared_ptr<Process>> & /*process_map*/,
    int /*current_time*/) {
  if (fifo_queue.empty())
    return -1;

  int candidate = fifo_queue.front();

  if (candidate >= 0 && candidate < static_cast<int>(frames.size())) {
    const Frame &frame = frames[candidate];
    if (frame.occupied) {
      return candidate;
    }
  }

  return -1;
}

void FIFOReplacement::on_page_access(int frame_id) {
  bool found = (std::find(fifo_queue.begin(), fifo_queue.end(), frame_id) !=
                fifo_queue.end());
  if (!found) {
    fifo_queue.push_back(frame_id);
  }
}

void FIFOReplacement::on_frame_release(int frame_id) {
  fifo_queue.remove(frame_id);
}

} // namespace OSSimulator
