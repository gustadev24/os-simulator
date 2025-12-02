"""
@file batch_processor.py
@brief Módulo para procesamiento por lotes de archivos de métricas.

Proporciona funcionalidad para generar diagramas de múltiples archivos
de métricas en una sola ejecución.
"""

import os
from pathlib import Path
from typing import List, Optional

from visualization.data_loader import MetricsLoader
from visualization.visualizer import MetricsVisualizer


class BatchProcessor:
    """
    @brief Procesador por lotes para generación de diagramas.

    Permite procesar múltiples archivos de métricas JSONL y generar
    diagramas para cada uno en subdirectorios separados.
    """

    def __init__(self, input_dir: str, output_dir: str):
        """
        @brief Constructor de BatchProcessor.
        @param input_dir Directorio con archivos de métricas JSONL.
        @param output_dir Directorio base de salida para diagramas.
        """
        self._input_dir = Path(input_dir)
        self._output_dir = Path(output_dir)
        self._processed_files: List[str] = []
        self._failed_files: List[str] = []

    @property
    def processed_files(self) -> List[str]:
        """
        @brief Obtiene la lista de archivos procesados exitosamente.
        @return Lista de rutas de archivos procesados.
        """
        return self._processed_files

    @property
    def failed_files(self) -> List[str]:
        """
        @brief Obtiene la lista de archivos que fallaron al procesar.
        @return Lista de rutas de archivos fallidos.
        """
        return self._failed_files

    def find_metrics_files(self, recursive: bool = True) -> List[Path]:
        """
        @brief Busca archivos de métricas JSONL en el directorio de entrada.
        @param recursive Si buscar recursivamente en subdirectorios.
        @return Lista de rutas a archivos JSONL encontrados.
        """
        if not self._input_dir.exists():
            return []

        pattern = "**/*.jsonl" if recursive else "*.jsonl"
        return sorted(self._input_dir.glob(pattern))

    def process_file(
        self, metrics_file: Path, output_subdir: Optional[str] = None
    ) -> bool:
        """
        @brief Procesa un archivo de métricas individual.
        @param metrics_file Ruta al archivo de métricas.
        @param output_subdir Subdirectorio de salida (si None, usa nombre del archivo).
        @return True si se procesó correctamente, False en caso contrario.
        """
        try:
            if output_subdir is None:
                output_subdir = metrics_file.stem

            output_path = self._output_dir / output_subdir

            visualizer = MetricsVisualizer(str(metrics_file), str(output_path))
            visualizer.generate_all()

            self._processed_files.append(str(metrics_file))
            return True

        except Exception as e:
            print(f"[ERROR] Error procesando {metrics_file}: {e}")
            self._failed_files.append(str(metrics_file))
            return False

    def process_all(self, recursive: bool = True) -> None:
        """
        @brief Procesa todos los archivos de métricas encontrados.
        @param recursive Si buscar recursivamente en subdirectorios.
        """
        metrics_files = self.find_metrics_files(recursive)

        if not metrics_files:
            print(f"[INFO] No se encontraron archivos JSONL en {self._input_dir}")
            return

        print(f"[INFO] Encontrados {len(metrics_files)} archivos de métricas")

        self._output_dir.mkdir(parents=True, exist_ok=True)

        for i, metrics_file in enumerate(metrics_files, 1):
            relative_path = metrics_file.relative_to(self._input_dir)
            output_subdir = str(relative_path.parent / relative_path.stem)

            print(f"\n[{i}/{len(metrics_files)}] Procesando: {relative_path}")
            self.process_file(metrics_file, output_subdir)
