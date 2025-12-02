from pathlib import Path
from typing import Dict, List, Any

import matplotlib.pyplot as plt

from visualization.base_generator import BaseGenerator
from visualization.data_loader import MetricsLoader


class ContextSwitchesGenerator(BaseGenerator):
    """
    @brief Generador de visualización de cambios de contexto.

    Muestra los cambios de contexto de CPU como puntos dispersos
    conectados por líneas a lo largo del tiempo.
    """

    def __init__(self, output_dir: Path):
        """
        @brief Constructor de ContextSwitchesGenerator.
        @param output_dir Directorio de salida para el gráfico.
        """
        super().__init__(output_dir)

    def generate(self, loader: MetricsLoader) -> None:
        """
        @brief Genera el gráfico de cambios de contexto.
        @param loader Instancia de MetricsLoader con datos cargados.
        """
        context_switches = loader.get_context_switches()

        if not context_switches:
            return

        fig, ax = plt.subplots(figsize=(14, 6))

        ticks = [cs["tick"] for cs in context_switches]
        processes = [cs["process"] for cs in context_switches]

        unique_procs = sorted(set(processes))
        proc_to_num = {p: i for i, p in enumerate(unique_procs)}
        colors = [self.get_process_color(p) for p in processes]

        ax.scatter(
            ticks,
            [proc_to_num[p] for p in processes],
            c=colors,
            s=self.STYLE["marker_size"],
            edgecolors="#374151",
            linewidth=self.STYLE["bar_edge_width"],
            zorder=3,
        )

        ax.plot(
            ticks,
            [proc_to_num[p] for p in processes],
            color=self.CHART_COLORS["neutral"],
            alpha=0.4,
            linewidth=1,
            linestyle="--",
            zorder=1,
        )

        ax.set_yticks(range(len(unique_procs)))
        ax.set_yticklabels(unique_procs, fontsize=self.FONT_SIZES["tick_label"])

        self.style_axis(
            ax,
            xlabel="Tiempo (ticks)",
            ylabel="Proceso",
            title=f"Cambios de Contexto (Total: {len(context_switches)})",
            grid_axis="x",
        )

        self.configure_axis_ticks(ax, x_data=ticks, integer_y=False)

        self.save_figure("08_context_switches.png")
