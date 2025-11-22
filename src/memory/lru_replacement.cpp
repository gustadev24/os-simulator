#include "memory/lru_replacement.hpp"
#include "core/process.hpp"
#include <limits>

namespace OSSimulator {

int LRUReplacement::select_victim(const std::vector<Frame>& frames, 
                                  const std::unordered_map<int, std::shared_ptr<Process>>& process_map,
                                  int /*current_time*/) {
    int victim = -1;
    int min_time = std::numeric_limits<int>::max();
    
    for (const auto& frame : frames) {
        if (!frame.occupied) continue;
        
        auto it = process_map.find(frame.process_id);
        if (it != process_map.end()) {
            const Page& page = it->second->page_table[frame.page_id];
            if (page.last_access_time < min_time) {
                min_time = page.last_access_time;
                victim = frame.frame_id;
            }
        }
    }
    return victim != -1 ? victim : 0;
}

}
