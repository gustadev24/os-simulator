#include "core/burst.hpp"

namespace OSSimulator {

Burst::Burst()
    : type(BurstType::CPU), duration(0), remaining_time(0), io_device("") {}

Burst::Burst(BurstType t, int d, const std::string &device)
    : type(t), duration(d), remaining_time(d), io_device(device) {}

bool Burst::is_completed() const { return remaining_time <= 0; }

void Burst::reset() { remaining_time = duration; }

} // namespace OSSimulator
