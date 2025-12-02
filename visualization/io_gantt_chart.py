from pathlib import Path
from typing import Dict, List, Any, Set, Tuple
from collections import defaultdict

import matplotlib.pyplot as plt
import matplotlib.patches as mpatches

from visualization.base_generator import BaseGenerator
from visualization.data_loader import MetricsLoader


class IOGanttChartGenerator(BaseGenerator):
    """
    @brief Generador de diagrama de Gantt para dispositivos de E/S.

    Visualiza la actividad de los dispositivos de E/S mostrando qué proceso
    está siendo atendido en cada tick versus cuáles están esperando en cola.

    Distingue visualmente entre:
    - Procesos activamente siendo atendidos por el dispositivo (color sólido)
    - Procesos esperando en la cola de E/S (color con patrón rayado)
    """

    def __init__(self, output_dir: Path):
        """
        @brief Constructor de IOGanttChartGenerator.
        @param output_dir Directorio de salida para el gráfico.
        """
        super().__init__(output_dir)

    def generate(self, loader: MetricsLoader) -> None:
        """
        @brief Genera el diagrama de Gantt de E/S.
        @param loader Instancia de MetricsLoader con datos cargados.
        """

        io_operations = loader.get_io_operations()

        if not io_operations:
            print("[INFO] No hay operaciones de E/S para visualizar")
            return

        io_periods = self._match_io_periods(io_operations)

        if not io_periods:
            print("[INFO] No se encontraron períodos de E/S completos")
            return

        io_device_events = loader.get_io_device_events()
        active_service_ticks = self._extract_active_service_ticks(io_device_events)

        max_tick = 0
        for periods in io_periods.values():
            for p in periods:
                max_tick = max(max_tick, p.get("end", 0))
        max_tick = max(max_tick, loader.get_max_tick())

        num_processes = len(io_periods)
        fig, ax = plt.subplots(figsize=(18, max(4, num_processes * 0.6 + 2)))

        y_pos = 0
        yticks = []
        yticklabels = []

        active_legend_added = False
        waiting_legend_added = False

        sorted_procs = self.sort_process_names(list(io_periods.keys()))

        for proc_name in sorted_procs:
            periods = io_periods[proc_name]

            for period in periods:
                start = period["start"]
                end = period["end"]
                duration = period["duration"]

                if duration <= 0:
                    continue

                segments = self._split_into_segments(
                    proc_name, start, end, active_service_ticks
                )

                for seg_start, seg_end, is_active in segments:
                    seg_duration = seg_end - seg_start
                    if seg_duration <= 0:
                        continue

                    color = self.get_process_color(proc_name)

                    if is_active:
                        label = "Atendido" if not active_legend_added else None
                        bar = ax.barh(
                            y_pos,
                            seg_duration,
                            left=seg_start,
                            height=0.7,
                            color=color,
                            alpha=0.9,
                            edgecolor="#374151",
                            linewidth=self.STYLE["bar_edge_width"],
                            label=label,
                        )
                        active_legend_added = True
                    else:
                        label = "En cola" if not waiting_legend_added else None
                        bar = ax.barh(
                            y_pos,
                            seg_duration,
                            left=seg_start,
                            height=0.7,
                            color=color,
                            alpha=0.4,
                            hatch="///",
                            edgecolor="#374151",
                            linewidth=self.STYLE["bar_edge_width"],
                            label=label,
                        )
                        waiting_legend_added = True

                if duration >= 3:
                    mid = start + duration / 2
                    ax.text(
                        mid,
                        y_pos,
                        f"{duration}",
                        ha="center",
                        va="center",
                        fontsize=self.FONT_SIZES["annotation"],
                        fontweight="bold",
                        color="black",
                    )

            yticks.append(y_pos)
            yticklabels.append(f"{proc_name} ({len(periods)} ops)")
            y_pos += 1

        ax.set_yticks(yticks)
        ax.set_yticklabels(
            yticklabels, fontsize=self.FONT_SIZES["tick_label"], fontweight="bold"
        )

        self.style_axis(
            ax,
            xlabel="Tiempo (ticks)",
            ylabel="Proceso",
            title="Diagrama de Gantt - Operaciones de E/S (Sólido=Atendido, Rayado=En Cola)",
            grid_axis="x",
        )

        self.configure_axis_ticks(
            ax, x_data=list(range(max_tick + 1)), integer_x=True, integer_y=False
        )
        ax.set_xlim(left=0)

        if active_legend_added or waiting_legend_added:
            ax.legend(loc="upper right", fontsize=self.FONT_SIZES["legend"])

        self.save_figure("07_io_gantt_chart.png")

    def _match_io_periods(
        self, io_operations: List[Dict[str, Any]]
    ) -> Dict[str, List[Dict[str, Any]]]:
        """
        @brief Empareja eventos de inicio y fin de E/S en períodos.
        @param io_operations Lista de operaciones de E/S (con tipo 'start' o 'end').
        @return Diccionario con nombre de proceso como clave y lista de períodos.
        """
        io_periods: Dict[str, List[Dict[str, Any]]] = defaultdict(list)
        pending_starts: Dict[str, List[int]] = defaultdict(list)

        sorted_ops = sorted(
            io_operations, key=lambda x: x.get("start", x.get("end", 0))
        )

        for op in sorted_ops:
            name = op["name"]
            if op["type"] == "start":
                pending_starts[name].append(op["start"])
            elif op["type"] == "end" and pending_starts[name]:
                start = pending_starts[name].pop(0)
                end = op["end"]
                io_periods[name].append(
                    {"start": start, "end": end, "duration": end - start}
                )

        return io_periods

    def _extract_active_service_ticks(
        self, io_device_events: List[Dict[str, Any]]
    ) -> Dict[str, Set[int]]:
        """
        @brief Extrae los ticks donde cada proceso está siendo activamente atendido.

        Usa los eventos STEP y COMPLETED del dispositivo para determinar qué proceso está
        siendo servido en cada tick.

        @param io_device_events Lista de eventos del dispositivo de E/S.
        @return Diccionario con nombre de proceso como clave y conjunto de ticks activos.
        """
        active_ticks: Dict[str, Set[int]] = defaultdict(set)

        if not io_device_events:
            return active_ticks

        for event in io_device_events:
            tick = event["tick"]
            event_type = event.get("event", "")
            name = event.get("name", "")

            if event_type in ("STEP", "COMPLETED") and name:
                active_ticks[name].add(tick)

        return active_ticks

    def _split_into_segments(
        self, proc_name: str, start: int, end: int, active_ticks: Dict[str, Set[int]]
    ) -> List[Tuple[int, int, bool]]:
        """
        @brief Divide un período de E/S en segmentos activos y en espera.

        @param proc_name Nombre del proceso.
        @param start Tick de inicio del período.
        @param end Tick de fin del período.
        @param active_ticks Diccionario de ticks activos por proceso.
        @return Lista de tuplas (start, end, is_active).
        """
        proc_active = active_ticks.get(proc_name, set())

        if not proc_active:
            return [(start, end, False)]

        segments = []
        current_start = start
        current_is_active = start in proc_active

        for tick in range(start + 1, end + 1):
            is_active = tick in proc_active

            if is_active != current_is_active:
                segments.append((current_start, tick, current_is_active))
                current_start = tick
                current_is_active = is_active

        if current_start < end:
            segments.append((current_start, end, current_is_active))

        return segments
