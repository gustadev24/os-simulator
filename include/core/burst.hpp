#ifndef BURST_HPP
#define BURST_HPP

#include <string>

namespace OSSimulator {

/**
 * Tipos de ráfaga que puede tener un proceso.
 */
enum class BurstType {
  CPU, //!< Ráfaga de CPU
  IO   //!< Ráfaga de E/S
};

/**
 * Representa una ráfaga de CPU o E/S de un proceso.
 */
struct Burst {
  BurstType type;        //!< Tipo de ráfaga (CPU o IO).
  int duration;          //!< Duración total de la ráfaga.
  int remaining_time;    //!< Tiempo restante para completar la ráfaga.
  std::string io_device; //!< Nombre del dispositivo de E/S (si aplica).

  /**
   * Constructor por defecto.
   */
  Burst();

  /**
   * Constructor parametrizado.
   * @param t Tipo de ráfaga.
   * @param d Duración de la ráfaga.
   * @param device Nombre del dispositivo de E/S (opcional).
   */
  Burst(BurstType t, int d, const std::string &device = "");

  /**
   * Indica si la ráfaga ha finalizado.
   * @return true si la ráfaga está completada.
   */
  bool is_completed() const;

  /**
   * Reinicia el estado de la ráfaga al valor inicial.
   */
  void reset();
};

} // namespace OSSimulator

#endif // BURST_HPP
