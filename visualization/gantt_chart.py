from pathlib import Path
from typing import Dict, List, Any

import seaborn as sns
import matplotlib.pyplot as plt

from visualization.base_generator import BaseGenerator
from visualization.data_loader import MetricsLoader


class GanttChartGenerator(BaseGenerator):
    """
    @brief Generador de diagramas de Gantt.
    
    Visualiza la ejecución de procesos mostrando los diferentes estados
    a lo largo del tiempo en un formato de barras horizontales.
    """
    
    STATE_STYLE = {
        'RUNNING': {'color': '#2ECC71', 'alpha': 0.9, 'hatch': None, 'label': 'Ejecución CPU'},
        'WAITING': {'color': '#E74C3C', 'alpha': 0.7, 'hatch': '///', 'label': 'Espera E/S'},
        'READY': {'color': '#3498DB', 'alpha': 0.5, 'hatch': None, 'label': 'Listo'},
        'MEMORY_WAITING': {'color': '#F39C12', 'alpha': 0.6, 'hatch': '\\\\\\', 'label': 'Espera Memoria'},
        'TERMINATED': {'color': '#95A5A6', 'alpha': 0.3, 'hatch': None, 'label': 'Terminado'},
        'NEW': {'color': '#9B59B6', 'alpha': 0.4, 'hatch': None, 'label': 'Nuevo'}
    }
    """@brief Estilos de visualización para cada estado de proceso."""
    
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
        
        y_pos = 0
        yticks = []
        yticklabels = []
        legend_added = set()
        
        for proc_name in loader.processes:
            states = sorted(process_states[proc_name], key=lambda x: x['tick'])
            
            for i, state_info in enumerate(states):
                start = state_info['tick']
                state = state_info['state']
                
                if i < len(states) - 1:
                    end = states[i + 1]['tick']
                else:
                    end = max_tick + 1
                
                duration = end - start
                
                if duration > 0 and state in self.STATE_STYLE:
                    style = self.STATE_STYLE[state]
                    color = style['color']
                    
                    label = style['label'] if style['label'] not in legend_added else None
                    if label:
                        legend_added.add(label)
                    
                    ax.barh(y_pos, duration, left=start, height=0.7,
                           color=color, alpha=style['alpha'], 
                           hatch=style['hatch'],
                           edgecolor='black', linewidth=0.5,
                           label=label)
            
            yticks.append(y_pos)
            yticklabels.append(proc_name)
            y_pos += 1
        
        ax.set_yticks(yticks)
        ax.set_yticklabels(yticklabels, fontsize=11, fontweight='bold')
        ax.set_xlabel('Tiempo (ticks)', fontsize=12, fontweight='bold')
        ax.set_ylabel('Procesos', fontsize=12, fontweight='bold')
        ax.set_title('Diagrama de Gantt - Ejecución de Procesos', fontsize=14, fontweight='bold')
        ax.grid(axis='x', alpha=0.3, linestyle='--')
        ax.set_xlim(left=0)
        ax.legend(loc='upper right', fontsize=9, ncol=2)
        
        self.save_figure('01_gantt_chart.png')
