from pathlib import Path
from typing import Dict, List, Any
from collections import defaultdict

import seaborn as sns
import matplotlib.pyplot as plt

from visualization.base_generator import BaseGenerator
from visualization.data_loader import MetricsLoader


class IOOperationsGenerator(BaseGenerator):
    """
    @brief Generador de visualización de operaciones de E/S.
    
    Muestra una línea temporal de las operaciones de E/S bloqueantes
    y la evolución del tamaño de la cola de E/S.
    """
    
    def __init__(self, output_dir: Path):
        """
        @brief Constructor de IOOperationsGenerator.
        @param output_dir Directorio de salida para el gráfico.
        """
        super().__init__(output_dir)
    
    def generate(self, loader: MetricsLoader) -> None:
        """
        @brief Genera el gráfico de operaciones de E/S.
        @param loader Instancia de MetricsLoader con datos cargados.
        """
        io_operations = loader.get_io_operations()
        
        if not io_operations:
            return
        
        io_periods = self._match_io_periods(io_operations)
        
        if not io_periods:
            return
        
        fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(14, 8), height_ratios=[2, 1])
        
        self._draw_io_timeline(ax1, io_periods)
        self._draw_io_queue_size(ax2, loader)
        
        self.save_figure('07_io_operations.png')
    
    def _match_io_periods(self, io_operations: List[Dict[str, Any]]) -> Dict[str, List[Dict[str, Any]]]:
        """
        @brief Empareja eventos de inicio y fin de E/S.
        @param io_operations Lista de operaciones de E/S.
        @return Diccionario con períodos de E/S por proceso.
        """
        io_periods: Dict[str, List[Dict[str, Any]]] = defaultdict(list)
        pending_starts: Dict[str, int] = {}
        
        for op in sorted(io_operations, key=lambda x: x.get('start', x.get('end', 0))):
            name = op['name']
            if op['type'] == 'start':
                pending_starts[name] = op['start']
            elif op['type'] == 'end' and name in pending_starts:
                start = pending_starts[name]
                end = op['end']
                io_periods[name].append({
                    'start': start, 
                    'end': end, 
                    'duration': end - start
                })
                del pending_starts[name]
        
        return io_periods
    
    def _draw_io_timeline(self, ax: plt.Axes, io_periods: Dict[str, List[Dict[str, Any]]]) -> None:
        """
        @brief Dibuja la línea temporal de operaciones de E/S.
        @param ax Objeto Axes de matplotlib.
        @param io_periods Diccionario con períodos de E/S por proceso.
        """
        y_pos = 0
        yticks = []
        yticklabels = []
        
        for proc_name in sorted(io_periods.keys()):
            periods = io_periods[proc_name]
            
            for period in periods:
                start = period['start']
                duration = period['duration']
                
                color = self.get_process_color(proc_name)
                ax.barh(y_pos, duration, left=start, height=0.6,
                       color=color, alpha=0.7, edgecolor='black', linewidth=1.5)
                
                mid = start + duration / 2
                ax.text(mid, y_pos, f'{duration}', ha='center', va='center',
                       fontsize=9, fontweight='bold', color='white')
            
            yticks.append(y_pos)
            yticklabels.append(f'{proc_name} ({len(periods)} ops)')
            y_pos += 1
        
        ax.set_yticks(yticks)
        ax.set_yticklabels(yticklabels)
        ax.set_xlabel('Tiempo (ticks)', fontsize=12, fontweight='bold')
        ax.set_ylabel('Proceso', fontsize=12, fontweight='bold')
        ax.set_title('Línea Temporal de Operaciones E/S (Períodos Bloqueados)', fontsize=14, fontweight='bold')
        ax.grid(axis='x', alpha=0.3, linestyle='--')
    
    def _draw_io_queue_size(self, ax: plt.Axes, loader: MetricsLoader) -> None:
        """
        @brief Dibuja el tamaño de la cola de E/S en el tiempo.
        @param ax Objeto Axes de matplotlib.
        @param loader Instancia de MetricsLoader con datos cargados.
        """
        queue_data = loader.get_queue_data()
        ticks = queue_data['ticks']
        sizes = queue_data['blocked_io']
        
        palette = sns.color_palette()
        
        if ticks:
            ax.step(ticks, sizes, where='post', color=palette[3], linewidth=2, label='Tamaño Cola E/S')
            ax.fill_between(ticks, 0, sizes, step='post', alpha=0.3, color=palette[3])
            ax.set_xlabel('Tiempo (ticks)', fontsize=12, fontweight='bold')
            ax.set_ylabel('Tamaño Cola', fontsize=12, fontweight='bold')
            ax.set_title('Tamaño de Cola de Bloqueados E/S', fontsize=14, fontweight='bold')
            ax.grid(True, alpha=0.3)
            ax.set_xlim(left=0)
