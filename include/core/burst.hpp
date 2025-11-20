#ifndef BURST_HPP
#define BURST_HPP

#include <string>

namespace OSSimulator {

enum class BurstType { CPU, IO };

struct Burst {
  BurstType type;
  int duration;
  int remaining_time;
  std::string io_device;

  Burst();
  Burst(BurstType t, int d, const std::string &device = "");
  
  bool is_completed() const;
  void reset();
};

} // namespace OSSimulator

#endif // BURST_HPP
