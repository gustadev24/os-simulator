#ifndef FIFO_REPLACEMENT_HPP
#define FIFO_REPLACEMENT_HPP

#include "memory/replacement_algorithm.hpp"
#include <list>

namespace OSSimulator {

/**
 * Implementación del algoritmo de reemplazo First-In, First-Out (FIFO).
 */
class FIFOReplacement : public ReplacementAlgorithm {
public:
    int select_victim(const std::vector<Frame>& frames, 
                      const std::unordered_map<int, std::shared_ptr<Process>>& process_map,
                      int current_time) override;

    void on_page_access(int frame_id) override;
    void on_frame_release(int frame_id) override;

private:
    std::list<int> fifo_queue; //!< Cola para mantener el orden de llegada de las páginas.
};

}

#endif
