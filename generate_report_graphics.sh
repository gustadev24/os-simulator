mkdir -p data/resultados/report

./build/bin/os_simulator -f data/procesos/report/processes_base.txt -c data/procesos/report/config_base.txt -m data/resultados/report/metrics_base.jsonl
./build/bin/os_simulator -f data/procesos/report/processes_concurrence.txt -c data/procesos/report/config_concurrence.txt -m data/resultados/report/metrics_concurrence.jsonl
./build/bin/os_simulator -f data/procesos/report/processes_cpu.txt -c data/procesos/report/config_cpu.txt -m data/resultados/report/metrics_cpu.jsonl
./build/bin/os_simulator -f data/procesos/report/processes_ram.txt -c data/procesos/report/config_ram.txt -m data/resultados/report/metrics_ram.jsonl

python3 -m visualization data/resultados/report/metrics_base.jsonl data/diagramas/report/base
python3 -m visualization data/resultados/report/metrics_concurrence.jsonl data/diagramas/report/concurrence
python3 -m visualization data/resultados/report/metrics_cpu.jsonl data/diagramas/report/cpu
python3 -m visualization data/resultados/report/metrics_ram.jsonl data/diagramas/report/ram

cp data/diagramas/report/base/01_gantt_chart.png report/figures/gantt_base.png
cp data/diagramas/report/concurrence/01_gantt_chart.png report/figures/gantt_concurrence.png
cp data/diagramas/report/cpu/01_gantt_chart.png report/figures/gantt_cpu.png
cp data/diagramas/report/ram/01_gantt_chart.png report/figures/gantt_ram.png