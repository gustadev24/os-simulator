import json
from pathlib import Path
from typing import List, Dict, Any, Set


class MetricsLoader:
    """
    @brief Clase encargada de cargar métricas desde archivos JSONL.

    Proporciona funcionalidad para leer archivos de métricas en formato JSONL
    y extraer información sobre procesos y eventos del sistema.
    """

    def __init__(self, metrics_file: str):
        """
        @brief Constructor de MetricsLoader.
        @param metrics_file Ruta al archivo de métricas en formato JSONL.
        """
        self._metrics_file = metrics_file
        self._metrics: List[Dict[str, Any]] = []
        self._processes: List[str] = []

    @property
    def metrics(self) -> List[Dict[str, Any]]:
        """
        @brief Obtiene la lista de métricas cargadas.
        @return Lista de diccionarios con las métricas.
        """
        return self._metrics

    @property
    def processes(self) -> List[str]:
        """
        @brief Obtiene la lista de procesos identificados.
        @return Lista de nombres de procesos ordenados.
        """
        return self._processes

    def load(self) -> None:
        """
        @brief Carga las métricas desde el archivo JSONL.

        Lee el archivo línea por línea, parseando cada línea como JSON.
        Identifica automáticamente todos los procesos presentes en los datos.
        """
        with open(self._metrics_file, "r") as f:
            self._metrics = [json.loads(line) for line in f if line.strip()]

        self._extract_processes()

    def _extract_processes(self) -> None:
        """
        @brief Extrae los nombres de procesos únicos de las métricas.

        Busca procesos en eventos de CPU y transiciones de estado.
        Ordena los procesos numéricamente (P1, P2, P10) en lugar de alfabéticamente.
        """
        processes: Set[str] = set()

        for event in self._metrics:
            if "cpu" in event and event["cpu"].get("pid", -1) > 0:
                processes.add(event["cpu"]["name"])
            if "state_transitions" in event:
                for trans in event["state_transitions"]:
                    processes.add(trans["name"])

        def sort_key(name: str):
            if name and len(name) > 1:
                try:
                    return int(name[1:])
                except ValueError:
                    pass
            return float("inf")

        self._processes = sorted(list(processes), key=sort_key)

    def get_max_tick(self) -> int:
        """
        @brief Obtiene el tick máximo registrado en las métricas.
        @return Valor del tick máximo.
        """
        return max(e.get("tick", 0) for e in self._metrics)

    def get_state_transitions(self) -> Dict[str, List[Dict[str, Any]]]:
        """
        @brief Extrae las transiciones de estado por proceso.
        @return Diccionario con nombre de proceso como clave y lista de transiciones.
        """
        process_states: Dict[str, List[Dict[str, Any]]] = {
            proc: [] for proc in self._processes
        }

        for event in self._metrics:
            tick = event.get("tick", -1)
            if "state_transitions" in event:
                for trans in event["state_transitions"]:
                    name = trans["name"]
                    to_state = trans["to"]
                    if name in process_states:
                        process_states[name].append({"tick": tick, "state": to_state})

        return process_states

    def get_queue_data(self) -> Dict[str, List[Any]]:
        """
        @brief Extrae datos de evolución de colas.
        @return Diccionario con ticks y conteos de cada tipo de cola.
        """
        ticks = []
        ready_counts = []
        blocked_memory_counts = []
        blocked_io_counts = []

        for event in self._metrics:
            if "queues" in event:
                tick = event.get("tick", -1)
                queues = event["queues"]

                ticks.append(tick)
                ready_counts.append(len(queues.get("ready", [])))
                blocked_memory_counts.append(len(queues.get("blocked_memory", [])))
                blocked_io_counts.append(len(queues.get("blocked_io", [])))

        return {
            "ticks": ticks,
            "ready": ready_counts,
            "blocked_memory": blocked_memory_counts,
            "blocked_io": blocked_io_counts,
        }

    def get_memory_data(self) -> Dict[str, List[Any]]:
        """
        @brief Extrae datos de uso de memoria.
        @return Diccionario con información de frames y fallos de página.
        """
        ticks = []
        used_frames = []
        page_faults = []

        for event in self._metrics:
            tick = event.get("tick", -1)

            if "frame_status" in event:
                frames = event["frame_status"]
                occupied = sum(1 for f in frames if f.get("occupied", False))
                ticks.append(tick)
                used_frames.append(occupied)

            if "memory" in event:
                mem = event["memory"]
                total_faults = mem.get("total_page_faults", 0)
                if total_faults > 0:
                    page_faults.append({"tick": tick, "total": total_faults})

        return {"ticks": ticks, "used_frames": used_frames, "page_faults": page_faults}

    def get_page_tables(self) -> Dict[str, Dict[str, Any]]:
        """
        @brief Obtiene las tablas de páginas finales por proceso.
        @return Diccionario con nombre de proceso y su tabla de páginas.
        """
        process_page_tables = {}

        for event in self._metrics:
            if "page_table" in event:
                pt = event["page_table"]
                proc_name = pt.get("name")
                if proc_name:
                    process_page_tables[proc_name] = pt

        return process_page_tables

    def get_final_frame_status(self) -> List[Dict[str, Any]]:
        """
        @brief Obtiene el estado final de los marcos de memoria.
        @return Lista de estados de marcos o lista vacía si no hay datos.
        """
        for event in reversed(self._metrics):
            if "frame_status" in event:
                return event["frame_status"]
        return []

    def get_context_switches(self) -> List[Dict[str, Any]]:
        """
        @brief Extrae los cambios de contexto de CPU.
        @return Lista de eventos de cambio de contexto.
        """
        context_switches = []

        for event in self._metrics:
            tick = event.get("tick", -1)
            if "cpu" in event:
                cpu = event["cpu"]
                if cpu.get("context_switch", False) and cpu.get("pid", -1) > 0:
                    context_switches.append(
                        {
                            "tick": tick,
                            "process": cpu.get("name"),
                            "event": cpu.get("event"),
                        }
                    )

        return context_switches

    def get_io_operations(self) -> List[Dict[str, Any]]:
        """
        @brief Extrae operaciones de E/S desde transiciones de estado.
        @return Lista de operaciones de E/S (inicio y fin).
        """
        io_operations = []

        for event in self._metrics:
            tick = event.get("tick", -1)

            if "state_transitions" in event:
                for trans in event["state_transitions"]:
                    if (
                        trans.get("to") == "WAITING"
                        and trans.get("reason") == "io_request"
                    ):
                        io_operations.append(
                            {"start": tick, "name": trans["name"], "type": "start"}
                        )
                    elif (
                        trans.get("from") == "WAITING"
                        and trans.get("reason") == "io_completed"
                    ):
                        io_operations.append(
                            {"end": tick, "name": trans["name"], "type": "end"}
                        )

        return io_operations

    def get_io_device_events(self) -> List[Dict[str, Any]]:
        """
        @brief Extrae eventos de dispositivos de E/S.
        @return Lista de eventos de dispositivos con tick, device, event, pid, name, remaining, queue.
        """
        io_device_events = []

        for event in self._metrics:
            tick = event.get("tick", -1)

            if "io" in event:
                io = event["io"]
                io_device_events.append(
                    {
                        "tick": tick,
                        "device": io.get("device", ""),
                        "event": io.get("event", ""),
                        "pid": io.get("pid", -1),
                        "name": io.get("name", ""),
                        "remaining": io.get("remaining", 0),
                        "queue_size": io.get("queue", 0),
                    }
                )

        return io_device_events

    def get_summary_metrics(self) -> Dict[str, Any]:
        """
        @brief Calcula métricas resumen de la simulación.
        @return Diccionario con métricas agregadas.
        """
        total_ticks = self.get_max_tick()
        total_context_switches = sum(
            1 for e in self._metrics if e.get("cpu", {}).get("context_switch", False)
        )

        total_page_faults = 0
        total_replacements = 0
        for event in self._metrics:
            if "memory" in event:
                total_page_faults = max(
                    total_page_faults, event["memory"].get("total_page_faults", 0)
                )
                total_replacements = max(
                    total_replacements, event["memory"].get("total_replacements", 0)
                )

        state_counts: Dict[str, int] = {}
        for event in self._metrics:
            if "state_transitions" in event:
                for trans in event["state_transitions"]:
                    state = trans["to"]
                    state_counts[state] = state_counts.get(state, 0) + 1

        return {
            "total_ticks": total_ticks,
            "total_context_switches": total_context_switches,
            "total_page_faults": total_page_faults,
            "total_replacements": total_replacements,
            "num_processes": len(self._processes),
            "state_counts": state_counts,
        }
