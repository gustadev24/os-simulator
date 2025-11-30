from pathlib import Path
from typing import Dict, List, Any
from collections import defaultdict

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
        
        fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(14, 8), sharex=True, gridspec_kw={'height_ratios': [2, 1]})

        self._draw_io_timeline(ax1, io_periods)
        self._draw_io_queue_size(ax2, loader)

        # Ensure both axes share the same x-range calculated from IO periods and queue ticks
        self._set_common_xrange(ax1, ax2, io_periods, loader)

        self.save_figure('06_io_operations.png')
    
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
                       color=color, alpha=0.8, edgecolor='#374151', 
                       linewidth=self.STYLE['bar_edge_width'])
                
                mid = start + duration / 2
                ax.text(mid, y_pos, f'{duration}', ha='center', va='center',
                       fontsize=self.FONT_SIZES['annotation'], fontweight='bold', color='white')
            
            yticks.append(y_pos)
            yticklabels.append(f'{proc_name} ({len(periods)} ops)')
            y_pos += 1
        
        ax.set_yticks(yticks)
        ax.set_yticklabels(yticklabels, fontsize=self.FONT_SIZES['tick_label'])
        self.style_axis(ax, xlabel='Tiempo (ticks)', ylabel='Proceso',
                       title='Línea Temporal de Operaciones E/S (Períodos Bloqueados)', grid_axis='x')
    
    def _draw_io_queue_size(self, ax: plt.Axes, loader: MetricsLoader) -> None:
        """
        @brief Dibuja el tamaño de la cola de E/S en el tiempo.
        @param ax Objeto Axes de matplotlib.
        @param loader Instancia de MetricsLoader con datos cargados.
        """
        queue_data = loader.get_queue_data()
        ticks = queue_data['ticks']
        sizes = queue_data['blocked_io']
        
        if ticks:
            ax.step(ticks, sizes, where='post', 
                   color=self.CHART_COLORS['quaternary'], linewidth=self.STYLE['line_width'], 
                   label='Tamaño Cola E/S')
            ax.fill_between(ticks, 0, sizes, step='post', 
                          alpha=self.STYLE['fill_alpha'], color=self.CHART_COLORS['quaternary'])
            self.style_axis(ax, xlabel='Tiempo (ticks)', ylabel='Tamaño Cola',
                          title='Tamaño de Cola de Bloqueados E/S')
            ax.set_xlim(left=0)
            self.configure_axis_ticks(ax, x_data=ticks, y_data=sizes)

    def _set_common_xrange(self, ax1: plt.Axes, ax2: plt.Axes, io_periods: Dict[str, List[Dict[str, Any]]], loader: MetricsLoader) -> None:
        """
        @brief Set the same x-axis limits for both axes using available IO periods and queue ticks.
        @param ax1 First Axes instance.
        @param ax2 Second Axes instance.
        @param io_periods Dict with IO periods by process.
        @param loader MetricsLoader instance (used to retrieve queue ticks).
        """
        xs: List[float] = []

        # gather start/end from io_periods
        for periods in io_periods.values():
            for p in periods:
                try:
                    xs.append(float(p.get('start', 0)))
                    xs.append(float(p.get('end', 0)))
                except Exception:
                    continue

        # gather ticks from queue data
        try:
            queue_data = loader.get_queue_data()
            ticks = queue_data.get('ticks', []) if isinstance(queue_data, dict) else []
            xs.extend([float(t) for t in ticks])
        except Exception:
            pass

        if not xs:
            return

        xmin = min(xs)
        xmax = max(xs)

        # add a small padding to the range
        span = xmax - xmin
        pad = span * 0.02 if span > 0 else 1

        left = max(0, xmin - pad)
        right = xmax + pad

        ax1.set_xlim(left, right)
        ax2.set_xlim(left, right)
