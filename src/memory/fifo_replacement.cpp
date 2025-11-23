#include "memory/fifo_replacement.hpp"
#include "core/process.hpp"
#include <algorithm>

namespace OSSimulator {

int FIFOReplacement::select_victim(const std::vector<Frame>& frames, 
                                   const std::unordered_map<int, std::shared_ptr<Process>>& process_map,
                                   int /*current_time*/) {
    if (fifo_queue.empty()) return -1;

    size_t attempts = fifo_queue.size();
    while (attempts--) {
        int candidate = fifo_queue.front();
        fifo_queue.pop_front();

        bool can_evict = true;
        if (candidate >= 0 && candidate < static_cast<int>(frames.size())) {
            const Frame &frame = frames[candidate];
            if (frame.occupied) {
                auto proc_it = process_map.find(frame.process_id);
                if (proc_it != process_map.end()) {
                    const auto &table = proc_it->second->page_table;
                    if (frame.page_id >= 0 && frame.page_id < static_cast<int>(table.size())) {
                        if (table[frame.page_id].referenced) {
                            can_evict = false;
                        }
                    }
                }
            }
        }

        if (can_evict) {
            return candidate;
        }

        fifo_queue.push_back(candidate);
    }

    return -1;
}

void FIFOReplacement::on_page_access(int frame_id) {
    // En FIFO, solo nos importa cuando se carga la página por primera vez.
    // Asumimos que on_page_access se llama al cargar.
    // Pero si se llama en cada acceso (lectura/escritura), debemos filtrar.
    // MemoryManager invoca este método únicamente cuando una página termina de cargarse.
    // Sin embargo, MemoryManager actual no distingue "load" de "access" en la interfaz actual.
    // Ajustaremos MemoryManager para llamar a on_page_load o similar, o usaremos lógica aquí.
    
    // Verificamos si ya está en la cola para no duplicar (aunque en FIFO puro solo se añade al cargar)
    bool found = (std::find(fifo_queue.begin(), fifo_queue.end(), frame_id) != fifo_queue.end());
    if (!found) {
        fifo_queue.push_back(frame_id);
    }
}

void FIFOReplacement::on_frame_release(int frame_id) {
    fifo_queue.remove(frame_id);
}

}
