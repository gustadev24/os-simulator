from pathlib import Path
from typing import Dict, List, Any

import matplotlib.pyplot as plt

from visualization.base_generator import BaseGenerator
from visualization.data_loader import MetricsLoader


class SummaryDashboardGenerator(BaseGenerator):
    """
    @brief Generador de dashboard resumen.

    Crea una tabla con métricas clave de la simulación.
    """

    def __init__(self, output_dir: Path):
        """
        @brief Constructor de SummaryDashboardGenerator.
        @param output_dir Directorio de salida para el gráfico.
        """
        super().__init__(output_dir)

    def generate(self, loader: MetricsLoader) -> None:
        """
        @brief Genera el dashboard resumen como tabla.
        @param loader Instancia de MetricsLoader con datos cargados.
        """
        summary = loader.get_summary_metrics()

        fig, ax = plt.subplots(figsize=(10, 6))
        ax.axis("off")

        fig.suptitle(
            "Resumen de la Simulación",
            fontsize=self.FONT_SIZES["title"] + 2,
            fontweight="bold",
            y=0.95,
        )

        table_data = [
            ["Tiempo Total", f"{summary['total_ticks']} ticks"],
            ["Procesos Simulados", str(summary["num_processes"])],
            ["Cambios de Contexto", str(summary["total_context_switches"])],
            ["Fallos de Página", str(summary["total_page_faults"])],
            ["Reemplazos de Página", str(summary["total_replacements"])],
        ]

        table = ax.table(
            cellText=table_data,
            colLabels=["Métrica", "Valor"],
            loc="center",
            cellLoc="left",
            colWidths=[0.5, 0.3],
        )

        table.auto_set_font_size(False)
        table.set_fontsize(self.FONT_SIZES["subtitle"])
        table.scale(1.2, 2.0)

        for (row, col), cell in table.get_celld().items():
            if row == 0:
                cell.set_text_props(fontweight="bold", color="white")
                cell.set_facecolor(self.TABLE_COLORS["header"])
            else:
                cell.set_facecolor(
                    self.TABLE_COLORS["row_even"]
                    if row % 2 == 0
                    else self.TABLE_COLORS["row_odd"]
                )
            cell.set_edgecolor(self.TABLE_COLORS["border"])

        self.save_figure("09_summary_dashboard.png")


class StateDistributionGenerator(BaseGenerator):
    """
    @brief Generador del gráfico de distribución de estados.

    Crea un gráfico de pastel con la distribución de transiciones de estado.
    """

    def __init__(self, output_dir: Path):
        """
        @brief Constructor de StateDistributionGenerator.
        @param output_dir Directorio de salida para el gráfico.
        """
        super().__init__(output_dir)

    def generate(self, loader: MetricsLoader) -> None:
        """
        @brief Genera el gráfico de distribución de estados.
        @param loader Instancia de MetricsLoader con datos cargados.
        """
        summary = loader.get_summary_metrics()
        state_counts = summary.get("state_counts", {})

        if not state_counts:
            return

        filtered_counts = {k: v for k, v in state_counts.items() if k != "NEW"}

        if not filtered_counts:
            return

        fig, ax = plt.subplots(figsize=(10, 8))

        labels = list(filtered_counts.keys())
        sizes = list(filtered_counts.values())
        colors = [self.get_state_color(label) for label in labels]

        wedges, texts, autotexts = ax.pie(
            sizes, autopct="%1.1f%%", colors=colors, startangle=90
        )

        for autotext in autotexts:
            autotext.set_color("white")
            autotext.set_fontweight("bold")
            autotext.set_fontsize(self.FONT_SIZES["tick_label"])

        ax.legend(
            wedges,
            labels,
            title="Estados",
            loc="center left",
            bbox_to_anchor=(1, 0.5),
            fontsize=self.FONT_SIZES["legend"],
        )

        ax.set_title(
            "Distribución de Transiciones de Estado",
            fontsize=self.FONT_SIZES["title"],
            fontweight="bold",
        )

        self.save_figure("10_state_distribution.png")
