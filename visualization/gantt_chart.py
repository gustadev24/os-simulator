from pathlib import Path
from typing import Dict, List, Any

import matplotlib.pyplot as plt

from visualization.base_generator import BaseGenerator
from visualization.data_loader import MetricsLoader


class GanttChartGenerator(BaseGenerator):
    """
    @brief Generador de diagramas de Gantt.

    Visualiza la ejecución de procesos mostrando los diferentes estados
    a lo largo del tiempo en un formato de barras horizontales.
    """

    def _get_state_style(self):
        """@brief Retorna estilos de visualización usando colores normalizados."""
        return {
            "RUNNING": {
                "color": self.STATE_COLORS["RUNNING"],
                "alpha": 0.9,
                "hatch": None,
                "label": "Ejecución CPU",
            },
            "WAITING": {
                "color": self.STATE_COLORS["WAITING"],
                "alpha": 0.8,
                "hatch": "///",
                "label": "Espera E/S",
            },
            "READY": {
                "color": self.STATE_COLORS["READY"],
                "alpha": 0.6,
                "hatch": None,
                "label": "Listo",
            },
            "MEMORY_WAITING": {
                "color": self.STATE_COLORS["MEMORY_WAITING"],
                "alpha": 0.7,
                "hatch": "\\\\\\",
                "label": "Espera Memoria",
            },
            "TERMINATED": {
                "color": self.STATE_COLORS["TERMINATED"],
                "alpha": 0.4,
                "hatch": None,
                "label": "Terminado",
            },
            "NEW": {
                "color": self.STATE_COLORS["NEW"],
                "alpha": 0.5,
                "hatch": None,
                "label": "Nuevo",
            },
        }

    def __init__(self, output_dir: Path):
        """
        @brief Constructor de GanttChartGenerator.
        @param output_dir Directorio de salida para el gráfico.
        """
        super().__init__(output_dir)

    def generate(self, loader: MetricsLoader) -> None:
        """
        @brief Genera el diagrama de Gantt.
        @param loader Instancia de MetricsLoader con datos cargados.
        """
        fig, ax = plt.subplots(figsize=(18, 7))

        process_states = loader.get_state_transitions()
        max_tick = loader.get_max_tick()
        state_styles = self._get_state_style()

        y_pos = 0
        yticks = []
        yticklabels = []
        legend_added = set()

        for proc_name in loader.processes:
            states = sorted(process_states[proc_name], key=lambda x: x["tick"])

            for i, state_info in enumerate(states):
                start = state_info["tick"]
                state = state_info["state"]

                if i < len(states) - 1:
                    end = states[i + 1]["tick"]
                else:
                    end = max_tick + 1

                duration = end - start

                if duration > 0 and state in state_styles:
                    style = state_styles[state]
                    color = style["color"]

                    label = (
                        style["label"] if style["label"] not in legend_added else None
                    )
                    if label:
                        legend_added.add(label)

                    ax.barh(
                        y_pos,
                        duration,
                        left=start,
                        height=0.7,
                        color=color,
                        alpha=style["alpha"],
                        hatch=style["hatch"],
                        edgecolor="#374151",
                        linewidth=self.STYLE["bar_edge_width"],
                        label=label,
                    )

            yticks.append(y_pos)
            yticklabels.append(proc_name)
            y_pos += 1

        ax.set_yticks(yticks)
        ax.set_yticklabels(
            yticklabels, fontsize=self.FONT_SIZES["tick_label"], fontweight="bold"
        )

        self.style_axis(
            ax,
            xlabel="Tiempo (ticks)",
            ylabel="Procesos",
            title="Diagrama de Gantt - Ejecución de Procesos",
            grid_axis="x",
        )

        self.configure_axis_ticks(
            ax, x_data=list(range(max_tick + 1)), integer_x=True, integer_y=False
        )
        ax.set_xlim(left=0)
        ax.legend(loc="upper right", fontsize=self.FONT_SIZES["legend"], ncol=2)

        self.save_figure("01_gantt_chart.png")
