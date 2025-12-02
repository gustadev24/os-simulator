import sys

from visualization.visualizer import MetricsVisualizer
from visualization.batch_processor import BatchProcessor


def print_usage():
    """
    @brief Imprime el mensaje de ayuda.
    """
    print("""
NOMBRE
    visualization - Generador de diagramas para métricas del simulador

SINOPSIS
    python -m visualization [archivo_metricas] [directorio_salida]
    python -m visualization --batch <directorio_entrada> [directorio_salida]

DESCRIPCIÓN
    Lee archivos de métricas en formato JSONL y genera diagramas de visualización.

MODOS DE OPERACIÓN
    Modo individual (por defecto):
        Procesa un único archivo de métricas.
        
    Modo por lotes (--batch):
        Procesa todos los archivos JSONL en un directorio y sus subdirectorios.

ARGUMENTOS
    archivo_metricas
        Ruta al archivo de métricas JSONL.
        Por defecto: data/resultados/metrics.jsonl

    directorio_salida
        Directorio donde se guardarán los diagramas.
        Por defecto: data/diagramas

OPCIONES
    --batch <directorio_entrada>
        Activa el modo por lotes. Procesa todos los archivos .jsonl
        encontrados en el directorio de entrada.

    -h, --help
        Muestra esta ayuda.

EJEMPLOS
    # Modo individual con configuración por defecto
    python -m visualization

    # Modo individual con archivo específico
    python -m visualization resultados/custom.jsonl

    # Modo individual con archivo y directorio de salida
    python -m visualization data/resultados/metrics.jsonl output/graficos

    # Modo por lotes - procesa todos los JSONL en un directorio
    python -m visualization --batch data/resultados/combinations/

    # Modo por lotes con directorio de salida personalizado
    python -m visualization --batch data/resultados/ output/diagramas_batch/
""")


def main():
    """
    @brief Función principal del módulo de visualización.

    Soporta dos modos de operación:<br>
    - Modo individual: procesa un único archivo de métricas<br>
    - Modo por lotes (--batch): procesa múltiples archivos en un directorio<br>
    """
    if len(sys.argv) > 1 and sys.argv[1] in ["-h", "--help"]:
        print_usage()
        return

    if len(sys.argv) > 1 and sys.argv[1] == "--batch":
        if len(sys.argv) < 3:
            print("[ERROR] Modo por lotes requiere directorio de entrada")
            print(
                "Uso: python -m visualization --batch <directorio_entrada> [directorio_salida]"
            )
            sys.exit(1)

        input_dir = sys.argv[2]
        output_dir = sys.argv[3] if len(sys.argv) > 3 else "data/diagramas/batch"

        processor = BatchProcessor(input_dir, output_dir)
        processor.process_all()
        return

    metrics_file = "data/resultados/metrics.jsonl"
    output_dir = "data/diagramas"

    if len(sys.argv) > 1:
        metrics_file = sys.argv[1]
    if len(sys.argv) > 2:
        output_dir = sys.argv[2]

    visualizer = MetricsVisualizer(metrics_file, output_dir)
    visualizer.generate_all()


if __name__ == "__main__":
    main()
