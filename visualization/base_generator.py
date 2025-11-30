from abc import ABC, abstractmethod
from pathlib import Path
from typing import List, Dict, Any

import seaborn as sns
import matplotlib.pyplot as plt


class BaseGenerator(ABC):
    """
    @brief Clase base abstracta para generadores de gráficos.
    
    Define la interfaz común y configuración compartida para todos
    los generadores de visualización.
    """
    
    STATE_COLORS = {
        'RUNNING': '#2ECC71',
        'READY': '#3498DB',
        'WAITING': '#E74C3C',
        'MEMORY_WAITING': '#F39C12',
        'TERMINATED': '#95A5A6',
        'NEW': '#9B59B6'
    }
    """@brief Paleta de colores para estados de procesos."""
    
    PROCESS_COLORS = {
        'P1': '#FF6B6B',
        'P2': '#4ECDC4',
        'P3': '#45B7D1',
        'P4': '#FFA07A',
        'P5': '#98D8C8',
    }
    """@brief Paleta de colores para procesos."""
    
    def __init__(self, output_dir: Path):
        """
        @brief Constructor de BaseGenerator.
        @param output_dir Directorio de salida para los gráficos generados.
        """
        self._output_dir = output_dir
        self._setup_seaborn()
    
    def _setup_seaborn(self) -> None:
        """
        @brief Configura los estilos por defecto de seaborn.
        """
        sns.set_theme()
        sns.set_context("notebook")
    
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
        return self.PROCESS_COLORS.get(process_name, '#95A5A6')
    
    def get_state_color(self, state: str) -> str:
        """
        @brief Obtiene el color asignado a un estado.
        @param state Nombre del estado.
        @return Código de color hexadecimal.
        """
        return self.STATE_COLORS.get(state, '#95A5A6')
    
    def save_figure(self, filename: str) -> None:
        """
        @brief Guarda la figura actual en el directorio de salida.
        @param filename Nombre del archivo (sin ruta).
        """
        plt.tight_layout()
        plt.savefig(self._output_dir / filename, dpi=300, bbox_inches='tight')
        plt.close()
