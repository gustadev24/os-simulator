from abc import ABC, abstractmethod
from pathlib import Path
from typing import List, Dict, Any

import seaborn as sns
import matplotlib.pyplot as plt
from matplotlib.ticker import MaxNLocator, MultipleLocator, AutoMinorLocator
import numpy as np


class BaseGenerator(ABC):
    """
    @brief Clase base abstracta para generadores de gráficos.
    
    Define la interfaz común y configuración compartida para todos
    los generadores de visualización.
    """
    
    # ==========================================================================
    # PALETA DE COLORES NORMALIZADA
    # ==========================================================================
    
    # Colores para estados de procesos (consistente en todos los gráficos)
    STATE_COLORS = {
        'RUNNING': '#22C55E',      # Verde esmeralda - ejecución activa
        'READY': '#3B82F6',        # Azul brillante - listo para ejecutar
        'WAITING': '#EF4444',      # Rojo coral - espera E/S
        'MEMORY_WAITING': '#F59E0B', # Ámbar - espera de memoria
        'TERMINATED': '#6B7280',   # Gris neutro - terminado
        'NEW': '#8B5CF6'           # Púrpura - nuevo
    }
    """@brief Paleta de colores para estados de procesos."""
    
    # Colores para procesos individuales (hasta 10 procesos)
    PROCESS_COLORS = {
        'P1': '#3B82F6',   # Azul
        'P2': '#22C55E',   # Verde
        'P3': '#F59E0B',   # Ámbar
        'P4': '#EF4444',   # Rojo
        'P5': '#8B5CF6',   # Púrpura
        'P6': '#06B6D4',   # Cian
        'P7': '#EC4899',   # Rosa
        'P8': '#14B8A6',   # Teal
        'P9': '#F97316',   # Naranja
        'P10': '#6366F1',  # Índigo
    }
    """@brief Paleta de colores para procesos."""
    
    # Colores para gráficos de líneas/áreas (colas, métricas)
    CHART_COLORS = {
        'primary': '#3B82F6',      # Azul - línea principal
        'secondary': '#22C55E',    # Verde - línea secundaria
        'tertiary': '#F59E0B',     # Ámbar - tercera línea
        'quaternary': '#EF4444',   # Rojo - cuarta línea
        'neutral': '#6B7280',      # Gris - elementos neutros
        'free': '#E5E7EB',         # Gris claro - recursos libres
    }
    """@brief Paleta de colores para gráficos de líneas."""
    
    # Colores para tablas y dashboard
    TABLE_COLORS = {
        'header': '#1F2937',       # Gris oscuro - encabezados
        'row_even': '#F9FAFB',     # Gris muy claro - filas pares
        'row_odd': '#FFFFFF',      # Blanco - filas impares
        'border': '#D1D5DB',       # Gris medio - bordes
    }
    """@brief Paleta de colores para tablas."""
    
    # ==========================================================================
    # ESTILOS NORMALIZADOS
    # ==========================================================================
    
    FONT_SIZES = {
        'title': 14,
        'subtitle': 12,
        'axis_label': 11,
        'tick_label': 10,
        'legend': 10,
        'annotation': 9,
    }
    """@brief Tamaños de fuente normalizados."""
    
    STYLE = {
        'grid_alpha': 0.3,
        'grid_style': '--',
        'line_width': 2.0,
        'bar_edge_width': 0.8,
        'marker_size': 100,
        'fill_alpha': 0.25,
    }
    """@brief Parámetros de estilo normalizados."""
    
    def __init__(self, output_dir: Path):
        """
        @brief Constructor de BaseGenerator.
        @param output_dir Directorio de salida para los gráficos generados.
        """
        self._output_dir = output_dir
        self._setup_style()
    
    def _setup_style(self) -> None:
        """
        @brief Configura los estilos por defecto de matplotlib y seaborn.
        """
        sns.set_theme(style='whitegrid', palette='husl')
        sns.set_context('notebook', font_scale=1.0)
        
        plt.rcParams.update({
            'font.family': 'sans-serif',
            'axes.titleweight': 'bold',
            'axes.labelweight': 'bold',
            'axes.titlesize': self.FONT_SIZES['title'],
            'axes.labelsize': self.FONT_SIZES['axis_label'],
            'xtick.labelsize': self.FONT_SIZES['tick_label'],
            'ytick.labelsize': self.FONT_SIZES['tick_label'],
            'legend.fontsize': self.FONT_SIZES['legend'],
            'figure.facecolor': 'white',
            'axes.facecolor': 'white',
            'axes.edgecolor': '#D1D5DB',
            'grid.color': '#E5E7EB',
            'grid.linestyle': '--',
            'grid.alpha': 0.5,
        })
    
    @abstractmethod
    def generate(self, loader: Any) -> None:
        """
        @brief Genera la visualización.
        @param loader Instancia de MetricsLoader con datos cargados.
        """
        pass
    
    def get_process_color(self, process_name: str) -> str:
        """
        @brief Obtiene el color asignado a un proceso.
        @param process_name Nombre del proceso.
        @return Código de color hexadecimal.
        """
        return self.PROCESS_COLORS.get(process_name, self.CHART_COLORS['neutral'])
    
    def get_state_color(self, state: str) -> str:
        """
        @brief Obtiene el color asignado a un estado.
        @param state Nombre del estado.
        @return Código de color hexadecimal.
        """
        return self.STATE_COLORS.get(state, self.CHART_COLORS['neutral'])
    
    def configure_axis_ticks(self, ax: plt.Axes, x_data: List = None, y_data: List = None,
                              integer_x: bool = True, integer_y: bool = True) -> None:
        """
        @brief Configura los ticks de los ejes de forma inteligente.
        
        Ajusta la densidad de ticks según el rango de datos para evitar
        aglomeración sin perder precisión.
        
        @param ax Objeto Axes de matplotlib.
        @param x_data Datos del eje X (opcional, para calcular rango).
        @param y_data Datos del eje Y (opcional, para calcular rango).
        @param integer_x Si True, fuerza ticks enteros en X.
        @param integer_y Si True, fuerza ticks enteros en Y.
        """
        if integer_x:
            ax.xaxis.set_major_locator(MaxNLocator(integer=True, nbins='auto', prune='both'))
        
        if integer_y:
            ax.yaxis.set_major_locator(MaxNLocator(integer=True, nbins='auto', prune='both'))
        
        # Ajustar número de ticks según rango de datos
        if x_data is not None and len(x_data) > 0:
            x_range = max(x_data) - min(x_data) if len(x_data) > 1 else 1
            if x_range > 100:
                ax.xaxis.set_major_locator(MaxNLocator(nbins=10, integer=integer_x))
            elif x_range > 50:
                ax.xaxis.set_major_locator(MaxNLocator(nbins=8, integer=integer_x))
            elif x_range <= 20:
                ax.xaxis.set_major_locator(MaxNLocator(nbins=min(int(x_range) + 1, 15), integer=integer_x))
        
        if y_data is not None and len(y_data) > 0:
            y_range = max(y_data) - min(y_data) if len(y_data) > 1 else 1
            if y_range > 100:
                ax.yaxis.set_major_locator(MaxNLocator(nbins=10, integer=integer_y))
            elif y_range > 50:
                ax.yaxis.set_major_locator(MaxNLocator(nbins=8, integer=integer_y))
            elif y_range <= 10:
                ax.yaxis.set_major_locator(MaxNLocator(nbins=min(int(y_range) + 1, 10), integer=integer_y))
    
    def style_axis(self, ax: plt.Axes, xlabel: str = None, ylabel: str = None,
                   title: str = None, grid: bool = True, grid_axis: str = 'both') -> None:
        """
        @brief Aplica estilos normalizados a un eje.
        
        @param ax Objeto Axes de matplotlib.
        @param xlabel Etiqueta del eje X.
        @param ylabel Etiqueta del eje Y.
        @param title Título del gráfico.
        @param grid Si True, muestra la cuadrícula.
        @param grid_axis Eje para la cuadrícula ('x', 'y', 'both').
        """
        if xlabel:
            ax.set_xlabel(xlabel, fontsize=self.FONT_SIZES['axis_label'], fontweight='bold')
        if ylabel:
            ax.set_ylabel(ylabel, fontsize=self.FONT_SIZES['axis_label'], fontweight='bold')
        if title:
            ax.set_title(title, fontsize=self.FONT_SIZES['title'], fontweight='bold', pad=10)
        
        if grid:
            ax.grid(True, axis=grid_axis, alpha=self.STYLE['grid_alpha'], 
                   linestyle=self.STYLE['grid_style'])
        else:
            ax.grid(False)
    
    def save_figure(self, filename: str) -> None:
        """
        @brief Guarda la figura actual en el directorio de salida.
        @param filename Nombre del archivo (sin ruta).
        """
        plt.tight_layout()
        plt.savefig(self._output_dir / filename, dpi=300, bbox_inches='tight',
                   facecolor='white', edgecolor='none')
        plt.close()
