from pathlib import Path
from typing import Dict, List, Any

import seaborn as sns
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle

from visualization.base_generator import BaseGenerator
from visualization.data_loader import MetricsLoader


class SummaryDashboardGenerator(BaseGenerator):
    """
    @brief Generador de dashboard resumen.
    
    Crea un panel con métricas clave de la simulación incluyendo
    tiempo total, cambios de contexto, fallos de página y distribución
    de transiciones de estado.
    """
    
    def __init__(self, output_dir: Path):
        """
        @brief Constructor de SummaryDashboardGenerator.
        @param output_dir Directorio de salida para el gráfico.
        """
        super().__init__(output_dir)
    
    def generate(self, loader: MetricsLoader) -> None:
        """
        @brief Genera el dashboard resumen.
        @param loader Instancia de MetricsLoader con datos cargados.
        """
        summary = loader.get_summary_metrics()
        
        fig = plt.figure(figsize=(16, 10))
        gs = fig.add_gridspec(3, 3, hspace=0.3, wspace=0.3)
        
        fig.suptitle('Dashboard Resumen de Simulación', fontsize=16, fontweight='bold')
        
        palette = sns.color_palette()
        
        metrics_data = [
            ('Tiempo Total', f'{summary["total_ticks"]} ticks', palette[0]),
            ('Cambios de Contexto', str(summary["total_context_switches"]), palette[3]),
            ('Fallos de Página', str(summary["total_page_faults"]), palette[1]),
            ('Reemplazos de Página', str(summary["total_replacements"]), palette[4]),
            ('Procesos', str(summary["num_processes"]), palette[2]),
        ]
        
        for idx, (label, value, color) in enumerate(metrics_data):
            row, col = idx // 3, idx % 3
            ax = fig.add_subplot(gs[row, col])
            ax.text(0.5, 0.6, value, ha='center', va='center', 
                   fontsize=32, fontweight='bold', color=color)
            ax.text(0.5, 0.3, label, ha='center', va='center',
                   fontsize=14, color='gray')
            ax.set_xlim(0, 1)
            ax.set_ylim(0, 1)
            ax.axis('off')
            ax.add_patch(Rectangle((0.05, 0.1), 0.9, 0.8, fill=False, 
                                  edgecolor=color, linewidth=3))
        
        state_counts = summary.get('state_counts', {})
        if state_counts:
            ax = fig.add_subplot(gs[2, :])
            labels = list(state_counts.keys())
            sizes = list(state_counts.values())
            pie_colors = [self.get_state_color(l) for l in labels]
            
            wedges, texts, autotexts = ax.pie(
                sizes, autopct='%1.1f%%',
                colors=pie_colors, startangle=90
            )
            for autotext in autotexts:
                autotext.set_color('white')
                autotext.set_fontweight('bold')
                autotext.set_fontsize(10)
            
            ax.legend(
                wedges, labels, title="Estados", loc="center left", 
                bbox_to_anchor=(1, 0, 0.5, 1), fontsize=10
            )
            
            ax.set_title('Distribución de Transiciones de Estado', fontsize=14, fontweight='bold')
        
        self.save_figure('09_summary_dashboard.png')
