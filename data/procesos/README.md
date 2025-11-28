# Quick Reference: OS Simulator Input Files

## Process File Format
```
# Comments start with #
PID arrival_time CPU(x),E/S(y),CPU(z) priority pages
```

### Example
```
P1 0 CPU(4),E/S(3),CPU(5) 1 4
P2 2 CPU(6) 2 5
P3 4 CPU(8),E/S(2),CPU(3) 3 6
```

## Config File Format
```
total_memory_frames=64
frame_size=4096
scheduling_algorithm=RoundRobin
page_replacement_algorithm=LRU
quantum=4
```

### Scheduling Algorithms
- FCFS (First Come First Served)
- SJF (Shortest Job First)
- RoundRobin
- Priority

### Page Replacement Algorithms
- FIFO
- LRU
- Optimal
- NRU

## Command Line Usage

### Load files
```bash
./build/bin/os_simulator -f <process_file> -c <config_file>
```

### Example
```bash
./build/bin/os_simulator -f data/procesos/procesos.txt -c data/procesos/config.txt
```

### Help
```bash
./build/bin/os_simulator -h
```

### Run demos
```bash
./build/bin/os_simulator
```

## Available Example Files

### Process Files
- `procesos.txt` - Specification example
- `procesos_simple.txt` - CPU only
- `procesos_extended.txt` - Complex bursts

### Config Files
- `config.txt` - RoundRobin + LRU
- `config_fcfs.txt` - FCFS + FIFO
- `config_sjf.txt` - SJF + FIFO
- `config_priority.txt` - Priority + LRU
