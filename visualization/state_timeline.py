from pathlib import Path
from typing import Dict, List, Any

import seaborn as sns
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches

from visualization.base_generator import BaseGenerator
from visualization.data_loader import MetricsLoader


class StateTimelineGenerator(BaseGenerator):
    """
    @brief Generador de visualización de línea temporal de estados.
    
    Muestra los estados de cada proceso a lo largo del tiempo
    usando barras horizontales coloreadas según el estado.
    """
    
    def __init__(self, output_dir: Path):
        """
        @brief Constructor de StateTimelineGenerator.
        @param output_dir Directorio de salida para el gráfico.
        """
        super().__init__(output_dir)
    
    def generate(self, loader: MetricsLoader) -> None:
        """
        @brief Genera el gráfico de línea temporal de estados.
        @param loader Instancia de MetricsLoader con datos cargados.
        """
        fig, ax = plt.subplots(figsize=(16, 8))
        
        process_states = loader.get_state_transitions()
        max_tick = loader.get_max_tick()
        
        y_pos = 0
        yticks = []
        yticklabels = []
        
        for proc_name in loader.processes:
            states = sorted(process_states[proc_name], key=lambda x: x['tick'])
            
            for i, state_info in enumerate(states):
                start = state_info['tick']
                end = states[i+1]['tick'] if i < len(states)-1 else start + 1
                duration = end - start
                
                if duration > 0:
                    color = self.get_state_color(state_info['state'])
                    ax.barh(y_pos, duration, left=start, height=0.7,
                           color=color, edgecolor='black', linewidth=0.5)
            
            yticks.append(y_pos)
            yticklabels.append(proc_name)
            y_pos += 1
        
        ax.set_yticks(yticks)
        ax.set_yticklabels(yticklabels)
        ax.set_xlabel('Tiempo (ticks)', fontsize=12, fontweight='bold')
        ax.set_ylabel('Procesos', fontsize=12, fontweight='bold')
        ax.set_title('Línea Temporal de Estados de Procesos', fontsize=14, fontweight='bold')
        ax.grid(axis='x', alpha=0.3, linestyle='--')
        
        legend_patches = [
            mpatches.Patch(color=color, label=state) 
            for state, color in self.STATE_COLORS.items()
        ]
        ax.legend(handles=legend_patches, loc='center left', bbox_to_anchor=(1, 0.5))
        
        self.save_figure('03_state_timeline.png')
