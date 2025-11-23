#include "memory/nru_replacement.hpp"
#include "core/process.hpp"
#include <vector>
#include <random>

namespace OSSimulator {

int NRUReplacement::select_victim(const std::vector<Frame>& frames, 
                                  const std::unordered_map<int, std::shared_ptr<Process>>& process_map,
                                  int /*current_time*/) {
    std::vector<int> classes[4];
    
    for (const auto& frame : frames) {
        if (!frame.occupied) continue;
        
        auto it = process_map.find(frame.process_id);
        if (it != process_map.end()) {
            const Page& page = it->second->page_table[frame.page_id];
            if (page.referenced) {
                continue;
            }
            int class_idx = 0;
            if (page.modified) class_idx += 1;
            classes[class_idx].push_back(frame.frame_id);
        }
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    
    for (int i = 0; i < 4; ++i) {
        if (!classes[i].empty()) {
            std::uniform_int_distribution<> dis(0, classes[i].size() - 1);
            return classes[i][dis(gen)];
        }
    }
    return -1;
}

}
