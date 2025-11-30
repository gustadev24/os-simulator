from pathlib import Path
from typing import Dict, List, Any

import matplotlib.pyplot as plt

from visualization.base_generator import BaseGenerator
from visualization.data_loader import MetricsLoader


class QueueEvolutionGenerator(BaseGenerator):
    """
    @brief Generador de visualización de evolución de colas.
    
    Muestra cómo cambian los tamaños de las colas de listos,
    bloqueados por memoria y bloqueados por E/S a lo largo del tiempo.
    """
    
    def __init__(self, output_dir: Path):
        """
        @brief Constructor de QueueEvolutionGenerator.
        @param output_dir Directorio de salida para el gráfico.
        """
        super().__init__(output_dir)
    
    def generate(self, loader: MetricsLoader) -> None:
        """
        @brief Genera el gráfico de evolución de colas.
        @param loader Instancia de MetricsLoader con datos cargados.
        """
        queue_data = loader.get_queue_data()
        
        ticks = queue_data['ticks']
        ready_counts = queue_data['ready']
        blocked_memory_counts = queue_data['blocked_memory']
        blocked_io_counts = queue_data['blocked_io']
        
        fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(14, 10), sharex=True)
        
        # Cola de Listos - color primario
        ax1.step(ticks, ready_counts, where='post', 
                color=self.CHART_COLORS['primary'], linewidth=self.STYLE['line_width'], 
                label='Cola de Listos')
        ax1.fill_between(ticks, 0, ready_counts, step='post', 
                        alpha=self.STYLE['fill_alpha'], color=self.CHART_COLORS['primary'])
        self.style_axis(ax1, ylabel='Tamaño', title='Evolución de Cola de Listos')
        ax1.set_ylim(bottom=0)
        ax1.legend(loc='upper right', fontsize=self.FONT_SIZES['legend'])
        
        # Cola de Bloqueados (Memoria) - color secundario
        ax2.step(ticks, blocked_memory_counts, where='post', 
                color=self.CHART_COLORS['tertiary'], linewidth=self.STYLE['line_width'], 
                label='Bloqueados (Memoria)')
        ax2.fill_between(ticks, 0, blocked_memory_counts, step='post', 
                        alpha=self.STYLE['fill_alpha'], color=self.CHART_COLORS['tertiary'])
        self.style_axis(ax2, ylabel='Tamaño', title='Evolución de Cola de Bloqueados (Memoria)')
        ax2.set_ylim(bottom=0)
        ax2.legend(loc='upper right', fontsize=self.FONT_SIZES['legend'])
        
        # Cola de Bloqueados (E/S) - color terciario
        ax3.step(ticks, blocked_io_counts, where='post', 
                color=self.CHART_COLORS['quaternary'], linewidth=self.STYLE['line_width'], 
                label='Bloqueados (E/S)')
        ax3.fill_between(ticks, 0, blocked_io_counts, step='post', 
                        alpha=self.STYLE['fill_alpha'], color=self.CHART_COLORS['quaternary'])
        self.style_axis(ax3, xlabel='Tiempo (ticks)', ylabel='Tamaño', 
                       title='Evolución de Cola de Bloqueados (E/S)')
        ax3.set_ylim(bottom=0)
        ax3.legend(loc='upper right', fontsize=self.FONT_SIZES['legend'])
        
        # Configure smart ticks for all axes
        all_counts = ready_counts + blocked_memory_counts + blocked_io_counts
        for ax in [ax1, ax2, ax3]:
            self.configure_axis_ticks(ax, x_data=ticks, y_data=all_counts)
        
        self.save_figure('02_queue_evolution.png')
