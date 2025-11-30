from pathlib import Path
from typing import Dict, List, Any

import seaborn as sns
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
        
        palette = sns.color_palette()
        
        ax1.step(ticks, ready_counts, where='post', color=palette[0], linewidth=2.5, label='Cola de Listos')
        ax1.fill_between(ticks, 0, ready_counts, step='post', alpha=0.3, color=palette[0])
        ax1.set_ylabel('Tamaño', fontsize=11, fontweight='bold')
        ax1.set_title('Evolución de Cola de Listos', fontsize=12, fontweight='bold')
        ax1.grid(True, alpha=0.3)
        ax1.set_ylim(bottom=0)
        ax1.legend(loc='upper right')
        
        ax2.step(ticks, blocked_memory_counts, where='post', color=palette[1], linewidth=2.5, label='Bloqueados (Memoria)')
        ax2.fill_between(ticks, 0, blocked_memory_counts, step='post', alpha=0.3, color=palette[1])
        ax2.set_ylabel('Tamaño', fontsize=11, fontweight='bold')
        ax2.set_title('Evolución de Cola de Bloqueados (Memoria)', fontsize=12, fontweight='bold')
        ax2.grid(True, alpha=0.3)
        ax2.set_ylim(bottom=0)
        ax2.legend(loc='upper right')
        
        ax3.step(ticks, blocked_io_counts, where='post', color=palette[2], linewidth=2.5, label='Bloqueados (E/S)')
        ax3.fill_between(ticks, 0, blocked_io_counts, step='post', alpha=0.3, color=palette[2])
        ax3.set_xlabel('Tiempo (ticks)', fontsize=12, fontweight='bold')
        ax3.set_ylabel('Tamaño', fontsize=11, fontweight='bold')
        ax3.set_title('Evolución de Cola de Bloqueados (E/S)', fontsize=12, fontweight='bold')
        ax3.grid(True, alpha=0.3)
        ax3.set_ylim(bottom=0)
        ax3.legend(loc='upper right')
        
        for ax in [ax1, ax2, ax3]:
            ax.yaxis.set_major_locator(plt.MaxNLocator(integer=True))
            ax.xaxis.set_major_locator(plt.MaxNLocator(integer=True))
        
        plt.setp(ax1.get_xticklabels(), visible=True)
        plt.setp(ax2.get_xticklabels(), visible=True)
        
        self.save_figure('02_queue_evolution.png')
