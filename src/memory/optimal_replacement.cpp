#include "memory/optimal_replacement.hpp"
#include "core/process.hpp"
#include <limits>

namespace OSSimulator {

int OptimalReplacement::select_victim(const std::vector<Frame>& frames, 
                                      const std::unordered_map<int, std::shared_ptr<Process>>& process_map,
                                      int /*current_time*/) {
    int victim = -1;
    int max_next_use = -1;
    
    for (const auto& frame : frames) {
        if (!frame.occupied) continue;
        
        auto it = process_map.find(frame.process_id);
        if (it != process_map.end()) {
            auto& proc = *it->second;
            int next_use = std::numeric_limits<int>::max();
            
            // Search in trace
            for (size_t i = proc.current_access_index; i < proc.memory_access_trace.size(); ++i) {
                if (proc.memory_access_trace[i] == frame.page_id) {
                    next_use = static_cast<int>(i);
                    break;
                }
            }
            
            if (next_use > max_next_use) {
                max_next_use = next_use;
                victim = frame.frame_id;
            }
        }
    }
    return victim != -1 ? victim : 0;
}

}
