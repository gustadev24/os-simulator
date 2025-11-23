#ifndef OPTIMAL_REPLACEMENT_HPP
#define OPTIMAL_REPLACEMENT_HPP

#include "memory/replacement_algorithm.hpp"

namespace OSSimulator {

/**
 * Implementación del algoritmo de reemplazo Óptimo.
 */
class OptimalReplacement : public ReplacementAlgorithm {
public:
  int select_victim(
      const std::vector<Frame> &frames,
      const std::unordered_map<int, std::shared_ptr<Process>> &process_map,
      int current_time) override;
};

} // namespace OSSimulator

#endif
