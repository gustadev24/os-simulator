import sys

from visualization.visualizer import MetricsVisualizer


def main():
    """
    @brief Función principal del módulo de visualización.
    
    Acepta parámetros opcionales desde línea de comandos:
    - Argumento 1: Ruta al archivo de métricas JSONL
    - Argumento 2: Directorio de salida para los gráficos
    """
    metrics_file = 'data/resultados/metrics.jsonl'
    output_dir = 'data/diagramas'
    
    if len(sys.argv) > 1:
        metrics_file = sys.argv[1]
    if len(sys.argv) > 2:
        output_dir = sys.argv[2]
    
    visualizer = MetricsVisualizer(metrics_file, output_dir)
    visualizer.generate_all()


if __name__ == '__main__':
    main()
