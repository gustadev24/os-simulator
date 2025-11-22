#ifndef REPLACEMENT_ALGORITHM_HPP
#define REPLACEMENT_ALGORITHM_HPP

#include "memory/page.hpp"
#include <vector>
#include <memory>
#include <unordered_map>

namespace OSSimulator {

struct Process;

/**
 * Estructura que representa un marco de memoria física.
 */
struct Frame {
    int frame_id;   //!< Identificador del marco.
    int process_id; //!< ID del proceso que ocupa el marco (-1 si está libre).
    int page_id;    //!< ID de la página almacenada.
    bool occupied;  //!< Indica si el marco está ocupado.
};

/**
 * Interfaz base para los algoritmos de reemplazo de páginas.
 */
class ReplacementAlgorithm {
public:
    virtual ~ReplacementAlgorithm() = default;

    /**
     * Selecciona la página víctima para ser reemplazada.
     * 
     * @param frames Lista de marcos de memoria.
     * @param process_map Mapa de procesos activos.
     * @param current_time Tiempo actual de la simulación.
     * @return ID del marco seleccionado como víctima.
     */
    virtual int select_victim(const std::vector<Frame>& frames, 
                              const std::unordered_map<int, std::shared_ptr<Process>>& process_map,
                              int current_time) = 0;

    /**
     * Notifica al algoritmo que una página ha sido cargada o accedida.
     * Útil para algoritmos como FIFO o LRU que necesitan mantener estado.
     * 
     * @param frame_id ID del marco accedido.
     */
    virtual void on_page_access(int frame_id) {}

    /**
     * Notifica al algoritmo que un marco ha sido liberado.
     * 
     * @param frame_id ID del marco liberado.
     */
    virtual void on_frame_release(int frame_id) {}
};

}

#endif
