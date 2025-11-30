#ifndef PAGE_HPP
#define PAGE_HPP

namespace OSSimulator {

/**
 * Representa una página lógica en el sistema.
 */
struct Page {
  int page_id;      //!< Identificador de la página.
  int frame_number; //!< Marco físico donde está cargada (-1 si no está en memoria).
  bool valid;       //!< Indica si la página está válida (cargada en memoria).
  bool modified;   //!< Bit de modificado (dirty).
  bool referenced; //!< Bit de referencia usado por algoritmos de reemplazo.
  int last_access_time; //!< Último tiempo de acceso (para políticas LRU/óptimas).
  int process_id;       //!< ID del proceso propietario de la página.

  /**
   * Constructor por defecto.
    */
  Page()
      : page_id(-1), frame_number(-1), valid(false), modified(false),
        referenced(false), last_access_time(0), process_id(-1) {}

  /**
   * Constructor parametrizado.
   *
   * @param id Identificador de la página.
   */
  Page(int id)
      : page_id(id), frame_number(-1), valid(false), modified(false),
        referenced(false), last_access_time(0), process_id(-1) {}
};

} // namespace OSSimulator

#endif
