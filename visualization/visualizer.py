import shutil
from pathlib import Path
from typing import List

from visualization.data_loader import MetricsLoader
from visualization.base_generator import BaseGenerator
from visualization.gantt_chart import GanttChartGenerator
from visualization.queue_evolution import QueueEvolutionGenerator
from visualization.state_timeline import StateTimelineGenerator
from visualization.memory_usage import MemoryUsageGenerator, PageTableGenerator, FrameAllocationGenerator
from visualization.io_operations import IOOperationsGenerator
from visualization.context_switches import ContextSwitchesGenerator
from visualization.summary_dashboard import SummaryDashboardGenerator


class MetricsVisualizer:
    """
    @brief Orquestador principal para generación de todas las visualizaciones.
    
    Coordina la carga de datos y la ejecución de todos los generadores
    de gráficos en secuencia.
    """
    
    def __init__(self, metrics_file: str, output_dir: str):
        """
        @brief Constructor de MetricsVisualizer.
        @param metrics_file Ruta al archivo de métricas JSONL.
        @param output_dir Directorio de salida para los gráficos.
        """
        self._metrics_file = metrics_file
        self._output_dir = Path(output_dir)
        self._loader = MetricsLoader(metrics_file)
        self._generators: List[BaseGenerator] = []
    
    def _setup_output_dir(self) -> None:
        """
        @brief Prepara el directorio de salida.
        
        Elimina el directorio si existe y lo crea vacío.
        """
        if self._output_dir.exists():
            shutil.rmtree(self._output_dir)
        self._output_dir.mkdir(parents=True, exist_ok=True)
    
    def _create_generators(self) -> None:
        """
        @brief Crea todas las instancias de generadores de gráficos.
        """
        self._generators = [
            GanttChartGenerator(self._output_dir),
            QueueEvolutionGenerator(self._output_dir),
            StateTimelineGenerator(self._output_dir),
            MemoryUsageGenerator(self._output_dir),
            PageTableGenerator(self._output_dir),
            FrameAllocationGenerator(self._output_dir),
            IOOperationsGenerator(self._output_dir),
            ContextSwitchesGenerator(self._output_dir),
            SummaryDashboardGenerator(self._output_dir),
        ]
    
    def generate_all(self) -> None:
        """
        @brief Genera todas las visualizaciones.
        
        Carga los datos, prepara el directorio de salida y ejecuta
        cada generador en secuencia.
        """
        print("\n" + "="*60)
        print("  GENERANDO DIAGRAMAS DE VISUALIZACIÓN")
        print("="*60 + "\n")
        
        self._loader.load()
        print(f"[INFO] Cargados {len(self._loader.metrics)} eventos")
        print(f"[INFO] Procesos encontrados: {', '.join(self._loader.processes)}")
        
        self._setup_output_dir()
        print(f"[INFO] Directorio de salida creado: {self._output_dir}")
        
        self._create_generators()
        
        generator_names = [
            "Diagrama de Gantt",
            "Evolución de Colas",
            "Línea Temporal de Estados",
            "Uso de Memoria",
            "Tablas de Páginas",
            "Asignación de Frames",
            "Operaciones E/S",
            "Cambios de Contexto",
            "Dashboard Resumen",
        ]
        
        for generator, name in zip(self._generators, generator_names):
            print(f"[INFO] Generando {name}...")
            try:
                generator.generate(self._loader)
                print(f"  ✓ {name} guardado")
            except Exception as e:
                print(f"  ⚠ Error generando {name}: {e}")
        
        print("\n" + "="*60)
        print(f"  ✓ Todos los diagramas generados en: {self._output_dir}")
        print("="*60 + "\n")
