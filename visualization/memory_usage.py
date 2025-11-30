from pathlib import Path
from typing import Dict, List, Any
from collections import defaultdict

import seaborn as sns
import matplotlib.pyplot as plt
import numpy as np

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
        
        palette = sns.color_palette()
        
        if ticks:
            ax1.fill_between(ticks, 0, used_frames, alpha=0.4, color=palette[0])
            ax1.plot(ticks, used_frames, color=palette[0], linewidth=2)
            ax1.set_ylabel('Frames Usados', fontsize=12, fontweight='bold')
            ax1.set_title('Uso de Frames de Memoria en el Tiempo', fontsize=14, fontweight='bold')
            ax1.grid(True, alpha=0.3)
            ax1.set_xlim(left=0)
        
        if page_faults:
            fault_ticks = [pf['tick'] for pf in page_faults]
            fault_totals = [pf['total'] for pf in page_faults]
            ax2.step(fault_ticks, fault_totals, where='post', color=palette[3], linewidth=2)
            ax2.fill_between(fault_ticks, 0, fault_totals, step='post', alpha=0.3, color=palette[3])
            ax2.set_xlabel('Tiempo (ticks)', fontsize=12, fontweight='bold')
            ax2.set_ylabel('Fallos de Página Totales', fontsize=12, fontweight='bold')
            ax2.set_title('Fallos de Página Acumulados', fontsize=14, fontweight='bold')
            ax2.grid(True, alpha=0.3)
            ax2.set_xlim(left=0)
        
        self.save_figure('04_memory_usage.png')


class PageTableGenerator(BaseGenerator):
    """
    @brief Generador de visualización de tablas de páginas.
    
    Muestra un heatmap de las tablas de páginas para cada proceso,
    incluyendo información de página, frame, validez y referencia.
    """
    
    def __init__(self, output_dir: Path):
        """
        @brief Constructor de PageTableGenerator.
        @param output_dir Directorio de salida para el gráfico.
        """
        super().__init__(output_dir)
    
    def generate(self, loader: MetricsLoader) -> None:
        """
        @brief Genera el heatmap de tablas de páginas.
        @param loader Instancia de MetricsLoader con datos cargados.
        """
        process_page_tables = loader.get_page_tables()
        
        if not process_page_tables:
            return
        
        num_processes = len(process_page_tables)
        fig, axes = plt.subplots(1, num_processes, figsize=(5*num_processes, 4))
        
        if num_processes == 1:
            axes = [axes]
        
        for idx, (proc_name, pt) in enumerate(sorted(process_page_tables.items())):
            pages = pt.get('pages', [])
            
            if not pages:
                continue
            
            num_pages = len(pages)
            data = np.zeros((num_pages, 4))
            
            for i, page in enumerate(pages):
                data[i, 0] = page.get('page', -1)
                data[i, 1] = page.get('frame', -1)
                data[i, 2] = 1 if page.get('valid', False) else 0
                data[i, 3] = 1 if page.get('referenced', False) else 0
            
            sns.heatmap(data.T, ax=axes[idx], cmap='RdYlGn', vmin=-1, vmax=15, 
                       cbar=False, annot=False)
            axes[idx].set_title(f'Tabla de Páginas - {proc_name}', fontweight='bold')
            axes[idx].set_yticks([0.5, 1.5, 2.5, 3.5])
            axes[idx].set_yticklabels(['Página', 'Frame', 'Válido', 'Ref'])
            axes[idx].set_xticks(range(num_pages))
            axes[idx].set_xlabel('Número de Página')
            
            for i in range(num_pages):
                for j in range(4):
                    val = int(data[i, j])
                    if j < 2:
                        text = str(val) if val >= 0 else '-'
                    else:
                        text = 'S' if val == 1 else 'N'
                    axes[idx].text(i + 0.5, j + 0.5, text, ha='center', va='center',
                                  color='black', fontsize=8, fontweight='bold')
        
        self.save_figure('05_page_tables.png')


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
        colors = [self.get_process_color(label) for label in labels[:-1]] + ['#ECF0F1']
        
        wedges, texts, autotexts = ax1.pie(
            sizes, labels=labels, autopct='%1.1f%%',
            colors=colors, startangle=90
        )
        for autotext in autotexts:
            autotext.set_color('white')
            autotext.set_fontweight('bold')
        
        ax1.set_title('Distribución de Frames de Memoria', fontsize=14, fontweight='bold')
        
        processes = sorted(frame_counts.keys())
        counts = [frame_counts[p] for p in processes]
        bars = ax2.bar(
            processes, counts, 
            color=[self.get_process_color(p) for p in processes],
            edgecolor='black', linewidth=1.5
        )
        
        ax2.set_ylabel('Frames Asignados', fontsize=12, fontweight='bold')
        ax2.set_title('Frames por Proceso', fontsize=14, fontweight='bold')
        ax2.grid(axis='y', alpha=0.3)
        
        for bar in bars:
            height = bar.get_height()
            ax2.text(
                bar.get_x() + bar.get_width()/2., height,
                f'{int(height)}',
                ha='center', va='bottom', fontweight='bold'
            )
        
        self.save_figure('06_frame_allocation.png')
