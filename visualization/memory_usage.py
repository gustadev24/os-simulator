from pathlib import Path
from typing import Dict, List, Any
from collections import defaultdict

import matplotlib.pyplot as plt

from visualization.base_generator import BaseGenerator
from visualization.data_loader import MetricsLoader


class MemoryUsageGenerator(BaseGenerator):
    """
    @brief Generador de visualización de uso de memoria.
    
    Muestra el uso de frames de memoria y la acumulación de
    fallos de página a lo largo del tiempo.
    """
    
    def __init__(self, output_dir: Path):
        """
        @brief Constructor de MemoryUsageGenerator.
        @param output_dir Directorio de salida para el gráfico.
        """
        super().__init__(output_dir)
    
    def generate(self, loader: MetricsLoader) -> None:
        """
        @brief Genera el gráfico de uso de memoria.
        @param loader Instancia de MetricsLoader con datos cargados.
        """
        memory_data = loader.get_memory_data()
        
        ticks = memory_data['ticks']
        used_frames = memory_data['used_frames']
        page_faults = memory_data['page_faults']
        
        fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(14, 10))
        
        if ticks:
            ax1.step(ticks, used_frames, where='post', 
                    color=self.CHART_COLORS['primary'], linewidth=self.STYLE['line_width'])
            ax1.fill_between(ticks, 0, used_frames, step='post', 
                           alpha=self.STYLE['fill_alpha'], color=self.CHART_COLORS['primary'])
            self.style_axis(ax1, xlabel='Tiempo (ticks)', ylabel='Frames Usados',
                          title='Uso de Frames de Memoria en el Tiempo')
            ax1.set_xlim(left=0)
            self.configure_axis_ticks(ax1, x_data=ticks, y_data=used_frames)
        
        if page_faults:
            fault_ticks = [pf['tick'] for pf in page_faults]
            fault_totals = [pf['total'] for pf in page_faults]
            ax2.step(fault_ticks, fault_totals, where='post', 
                    color=self.CHART_COLORS['quaternary'], linewidth=self.STYLE['line_width'])
            ax2.fill_between(fault_ticks, 0, fault_totals, step='post', 
                           alpha=self.STYLE['fill_alpha'], color=self.CHART_COLORS['quaternary'])
            self.style_axis(ax2, xlabel='Tiempo (ticks)', ylabel='Fallos de Página Totales',
                          title='Fallos de Página Acumulados')
            ax2.set_xlim(left=0)
            self.configure_axis_ticks(ax2, x_data=fault_ticks, y_data=fault_totals)
        
        self.save_figure('03_memory_usage.png')


class PageTableGenerator(BaseGenerator):
    """
    @brief Generador de visualización de tablas de páginas.
    
    Muestra una tabla simple de las páginas y frames asignados para cada proceso.
    """
    
    def __init__(self, output_dir: Path):
        """
        @brief Constructor de PageTableGenerator.
        @param output_dir Directorio de salida para el gráfico.
        """
        super().__init__(output_dir)
    
    def generate(self, loader: MetricsLoader) -> None:
        """
        @brief Genera la visualización de tablas de páginas.
        @param loader Instancia de MetricsLoader con datos cargados.
        """
        process_page_tables = loader.get_page_tables()
        
        if not process_page_tables:
            return
        
        num_processes = len(process_page_tables)
        fig, axes = plt.subplots(1, num_processes, figsize=(4*num_processes, 5))
        
        if num_processes == 1:
            axes = [axes]
        
        for idx, (proc_name, pt) in enumerate(sorted(process_page_tables.items())):
            pages = pt.get('pages', [])
            
            if not pages:
                continue
            
            table_data = []
            for page in pages:
                page_num = page.get('page', -1)
                frame_num = page.get('frame', -1)
                frame_str = str(frame_num) if frame_num >= 0 else '-'
                table_data.append([str(page_num), frame_str])
            
            axes[idx].axis('off')
            
            table = axes[idx].table(
                cellText=table_data,
                colLabels=['Página', 'Frame'],
                loc='center',
                cellLoc='center',
                colWidths=[0.4, 0.4]
            )
            
            table.auto_set_font_size(False)
            table.set_fontsize(self.FONT_SIZES['tick_label'])
            table.scale(1.2, 1.8)
            
            proc_color = self.get_process_color(proc_name)
            
            for (row, col), cell in table.get_celld().items():
                if row == 0:
                    cell.set_text_props(fontweight='bold', color='white')
                    cell.set_facecolor(proc_color)
                else:
                    cell.set_facecolor(self.TABLE_COLORS['row_even'] if row % 2 == 0 else self.TABLE_COLORS['row_odd'])
                cell.set_edgecolor(self.TABLE_COLORS['border'])
            
            axes[idx].set_title(f'Tabla de Páginas - {proc_name}', 
                              fontweight='bold', fontsize=self.FONT_SIZES['subtitle'], pad=20)
        
        fig.suptitle('Tablas de Páginas por Proceso', fontsize=self.FONT_SIZES['title'], fontweight='bold', y=1.02)
        
        self.save_figure('04_page_tables.png')


class FrameAllocationGenerator(BaseGenerator):
    """
    @brief Generador de visualización de asignación de marcos.
    
    Muestra la distribución de marcos de memoria entre procesos
    mediante gráficos de pastel y barras.
    """
    
    def __init__(self, output_dir: Path):
        """
        @brief Constructor de FrameAllocationGenerator.
        @param output_dir Directorio de salida para el gráfico.
        """
        super().__init__(output_dir)
    
    def generate(self, loader: MetricsLoader) -> None:
        """
        @brief Genera el gráfico de asignación de marcos.
        @param loader Instancia de MetricsLoader con datos cargados.
        """
        final_frame_status = loader.get_final_frame_status()
        
        if not final_frame_status:
            return
        
        frame_counts: Dict[str, int] = defaultdict(int)
        free_frames = 0
        
        for frame in final_frame_status:
            if frame.get('occupied', False):
                pid = frame.get('pid', -1)
                proc_name = f"P{pid}"
                frame_counts[proc_name] += 1
            else:
                free_frames += 1
        
        fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))
        
        labels = list(frame_counts.keys()) + ['Libre']
        sizes = list(frame_counts.values()) + [free_frames]
        colors = [self.get_process_color(label) for label in labels[:-1]] + [self.CHART_COLORS['free']]
        
        wedges, texts, autotexts = ax1.pie(
            sizes, labels=labels, autopct='%1.1f%%',
            colors=colors, startangle=90
        )
        for autotext in autotexts:
            autotext.set_color('white')
            autotext.set_fontweight('bold')
            autotext.set_fontsize(self.FONT_SIZES['annotation'])
        
        ax1.set_title('Distribución de Frames de Memoria', fontsize=self.FONT_SIZES['title'], fontweight='bold')
        
        processes = sorted(frame_counts.keys())
        counts = [frame_counts[p] for p in processes]
        bars = ax2.bar(
            processes, counts, 
            color=[self.get_process_color(p) for p in processes],
            edgecolor='#374151', linewidth=self.STYLE['bar_edge_width']
        )
        
        self.style_axis(ax2, ylabel='Frames Asignados', title='Frames por Proceso', grid_axis='y')
        self.configure_axis_ticks(ax2, y_data=counts, integer_x=False)
        
        for bar in bars:
            height = bar.get_height()
            ax2.text(
                bar.get_x() + bar.get_width()/2., height,
                f'{int(height)}',
                ha='center', va='bottom', fontweight='bold', fontsize=self.FONT_SIZES['annotation']
            )
        
        self.save_figure('05_frame_allocation.png')
