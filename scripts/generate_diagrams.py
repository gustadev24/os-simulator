#!/usr/bin/env python3
"""
Sistema de Visualización de Métricas del Simulador de SO
Genera diagramas completos basados en los datos JSONL recopilados.
"""

import json
import os
import shutil
from pathlib import Path
from typing import List, Dict, Any
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from matplotlib.patches import Rectangle
import numpy as np
from collections import defaultdict

class MetricsVisualizer:
    def __init__(self, metrics_file: str, output_dir: str):
        self.metrics_file = metrics_file
        self.output_dir = Path(output_dir)
        self.metrics = []
        self.processes = set()
        
        # Color scheme for processes
        self.process_colors = {
            'P1': '#FF6B6B',  # Red
            'P2': '#4ECDC4',  # Teal
            'P3': '#45B7D1',  # Blue
            'P4': '#FFA07A',  # Light Salmon
            'P5': '#98D8C8',  # Mint
        }
        
        self.state_colors = {
            'RUNNING': '#2ECC71',      # Green
            'READY': '#3498DB',        # Blue
            'WAITING': '#E74C3C',      # Red
            'MEMORY_WAITING': '#F39C12',  # Orange
            'TERMINATED': '#95A5A6',   # Gray
            'NEW': '#9B59B6'           # Purple
        }
        
    def load_metrics(self):
        """Carga las métricas desde el archivo JSONL"""
        with open(self.metrics_file, 'r') as f:
            self.metrics = [json.loads(line) for line in f if line.strip()]
        
        # Identificar todos los procesos
        for event in self.metrics:
            if 'cpu' in event and event['cpu'].get('pid', -1) > 0:
                self.processes.add(event['cpu']['name'])
            if 'state_transitions' in event:
                for trans in event['state_transitions']:
                    self.processes.add(trans['name'])
        
        self.processes = sorted(list(self.processes))
        print(f"[INFO] Loaded {len(self.metrics)} events")
        print(f"[INFO] Found processes: {', '.join(self.processes)}")
    
    def clear_output_dir(self):
        """Limpia el directorio de salida"""
        if self.output_dir.exists():
            shutil.rmtree(self.output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)
        print(f"[INFO] Output directory created: {self.output_dir}")
    
    def generate_gantt_chart(self):
        """Diagrama de Gantt mostrando la ejecución de procesos"""
        print("[INFO] Generating Gantt chart...")
        
        fig, ax = plt.subplots(figsize=(18, 7))
        
        # Track state for each process over time
        process_states = {proc: [] for proc in self.processes}
        
        for event in self.metrics:
            tick = event.get('tick', -1)
            if 'state_transitions' in event:
                for trans in event['state_transitions']:
                    name = trans['name']
                    to_state = trans['to']
                    if name in process_states:
                        process_states[name].append({
                            'tick': tick,
                            'state': to_state
                        })
        
        # Define colors and styles for each state
        state_style = {
            'RUNNING': {'color': '#2ECC71', 'alpha': 0.9, 'hatch': None, 'label': 'CPU Execution'},
            'WAITING': {'color': '#E74C3C', 'alpha': 0.7, 'hatch': '///', 'label': 'I/O Wait'},
            'READY': {'color': '#3498DB', 'alpha': 0.5, 'hatch': None, 'label': 'Ready'},
            'MEMORY_WAITING': {'color': '#F39C12', 'alpha': 0.6, 'hatch': '\\\\\\', 'label': 'Memory Wait'},
            'TERMINATED': {'color': '#95A5A6', 'alpha': 0.3, 'hatch': None, 'label': 'Terminated'},
            'NEW': {'color': '#9B59B6', 'alpha': 0.4, 'hatch': None, 'label': 'New'}
        }
        
        # Draw bars for each process
        y_pos = 0
        yticks = []
        yticklabels = []
        
        # Track which states we've added to legend
        legend_added = set()
        
        for proc_name in self.processes:
            states = sorted(process_states[proc_name], key=lambda x: x['tick'])
            
            for i, state_info in enumerate(states):
                start = state_info['tick']
                state = state_info['state']
                
                # Find end time (next state transition or max tick + 1)
                if i < len(states) - 1:
                    end = states[i + 1]['tick']
                else:
                    # Last state - extend a bit for visibility
                    end = max(e.get('tick', 0) for e in self.metrics) + 1
                
                duration = end - start
                
                if duration > 0 and state in state_style:
                    style = state_style[state]
                    
                    # Always use state color (not process color)
                    color = style['color']
                    
                    label = style['label'] if style['label'] not in legend_added else None
                    if label:
                        legend_added.add(label)
                    
                    ax.barh(y_pos, duration, left=start, height=0.7,
                           color=color, alpha=style['alpha'], 
                           hatch=style['hatch'],
                           edgecolor='black', linewidth=0.5,
                           label=label)
            
            yticks.append(y_pos)
            yticklabels.append(proc_name)
            y_pos += 1
        
        ax.set_yticks(yticks)
        ax.set_yticklabels(yticklabels, fontsize=11, fontweight='bold')
        ax.set_xlabel('Time (ticks)', fontsize=12, fontweight='bold')
        ax.set_ylabel('Processes', fontsize=12, fontweight='bold')
        ax.set_title('Process Execution Gantt Chart (All States)', fontsize=14, fontweight='bold')
        ax.grid(axis='x', alpha=0.3, linestyle='--')
        ax.set_xlim(left=0)
        
        # Legend
        ax.legend(loc='upper right', fontsize=9, ncol=2)
        
        plt.tight_layout()
        plt.savefig(self.output_dir / '01_gantt_chart.png', dpi=300, bbox_inches='tight')
        plt.close()
        print("  ✓ Gantt chart saved")
    
    def generate_queue_evolution(self):
        """Evolución de las colas del sistema"""
        print("[INFO] Generating queue evolution...")
        
        ticks = []
        ready_counts = []
        blocked_memory_counts = []
        blocked_io_counts = []
        
        for event in self.metrics:
            if 'queues' in event:
                tick = event.get('tick', -1)
                queues = event['queues']
                
                ticks.append(tick)
                ready_counts.append(len(queues.get('ready', [])))
                blocked_memory_counts.append(len(queues.get('blocked_memory', [])))
                blocked_io_counts.append(len(queues.get('blocked_io', [])))
        
        fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(14, 10), sharex=True)
        
        # Ready Queue
        ax1.step(ticks, ready_counts, where='post', color='#3498DB', linewidth=2.5, label='Ready Queue')
        ax1.fill_between(ticks, 0, ready_counts, step='post', alpha=0.3, color='#3498DB')
        ax1.set_ylabel('Queue Size', fontsize=11, fontweight='bold')
        ax1.set_title('Ready Queue Evolution', fontsize=12, fontweight='bold')
        ax1.grid(True, alpha=0.3)
        ax1.set_ylim(bottom=0)
        ax1.legend(loc='upper right')
        
        # Blocked (Memory) Queue
        ax2.step(ticks, blocked_memory_counts, where='post', color='#F39C12', linewidth=2.5, label='Blocked (Memory)')
        ax2.fill_between(ticks, 0, blocked_memory_counts, step='post', alpha=0.3, color='#F39C12')
        ax2.set_ylabel('Queue Size', fontsize=11, fontweight='bold')
        ax2.set_title('Blocked (Memory) Queue Evolution', fontsize=12, fontweight='bold')
        ax2.grid(True, alpha=0.3)
        ax2.set_ylim(bottom=0)
        ax2.legend(loc='upper right')
        
        # Blocked (I/O) Queue
        ax3.step(ticks, blocked_io_counts, where='post', color='#E74C3C', linewidth=2.5, label='Blocked (I/O)')
        ax3.fill_between(ticks, 0, blocked_io_counts, step='post', alpha=0.3, color='#E74C3C')
        ax3.set_xlabel('Time (ticks)', fontsize=12, fontweight='bold')
        ax3.set_ylabel('Queue Size', fontsize=11, fontweight='bold')
        ax3.set_title('Blocked (I/O) Queue Evolution', fontsize=12, fontweight='bold')
        ax3.grid(True, alpha=0.3)
        ax3.set_ylim(bottom=0)
        ax3.legend(loc='upper right')
        
        # Set integer ticks for all axes
        for ax in [ax1, ax2, ax3]:
            ax.yaxis.set_major_locator(plt.MaxNLocator(integer=True))
            ax.xaxis.set_major_locator(plt.MaxNLocator(integer=True))
        
        # Show x-axis labels on all charts (not just bottom)
        # Remove sharex to allow independent tick labels
        plt.setp(ax1.get_xticklabels(), visible=True)
        plt.setp(ax2.get_xticklabels(), visible=True)
        
        plt.tight_layout()
        plt.savefig(self.output_dir / '02_queue_evolution.png', dpi=300, bbox_inches='tight')
        plt.close()
        print("  ✓ Queue evolution saved")
    
    def generate_state_diagram(self):
        """Diagrama de estados de los procesos"""
        print("[INFO] Generating process state timeline...")
        
        fig, ax = plt.subplots(figsize=(16, 8))
        
        # Track state for each process
        process_states = {proc: [] for proc in self.processes}
        
        for event in self.metrics:
            tick = event.get('tick', -1)
            if 'state_transitions' in event:
                for trans in event['state_transitions']:
                    name = trans['name']
                    to_state = trans['to']
                    if name in process_states:
                        process_states[name].append({
                            'tick': tick,
                            'state': to_state
                        })
        
        # Draw state timeline for each process
        y_pos = 0
        yticks = []
        yticklabels = []
        
        for proc_name in self.processes:
            states = sorted(process_states[proc_name], key=lambda x: x['tick'])
            
            for i, state_info in enumerate(states):
                start = state_info['tick']
                end = states[i+1]['tick'] if i < len(states)-1 else start + 1
                duration = end - start
                
                if duration > 0:
                    color = self.state_colors.get(state_info['state'], '#95A5A6')
                    ax.barh(y_pos, duration, left=start, height=0.7,
                           color=color, edgecolor='black', linewidth=0.5)
            
            yticks.append(y_pos)
            yticklabels.append(proc_name)
            y_pos += 1
        
        ax.set_yticks(yticks)
        ax.set_yticklabels(yticklabels)
        ax.set_xlabel('Time (ticks)', fontsize=12, fontweight='bold')
        ax.set_ylabel('Processes', fontsize=12, fontweight='bold')
        ax.set_title('Process State Timeline', fontsize=14, fontweight='bold')
        ax.grid(axis='x', alpha=0.3, linestyle='--')
        
        # Legend
        legend_patches = [mpatches.Patch(color=color, label=state) 
                         for state, color in self.state_colors.items()]
        ax.legend(handles=legend_patches, loc='center left', bbox_to_anchor=(1, 0.5))
        
        plt.tight_layout()
        plt.savefig(self.output_dir / '03_state_timeline.png', dpi=300, bbox_inches='tight')
        plt.close()
        print("  ✓ State timeline saved")
    
    def generate_memory_usage(self):
        """Uso de memoria a lo largo del tiempo"""
        print("[INFO] Generating memory usage visualization...")
        
        ticks = []
        used_frames = []
        page_faults = []
        
        for event in self.metrics:
            tick = event.get('tick', -1)
            
            if 'frame_status' in event:
                frames = event['frame_status']
                occupied = sum(1 for f in frames if f.get('occupied', False))
                ticks.append(tick)
                used_frames.append(occupied)
            
            if 'memory' in event:
                mem = event['memory']
                total_faults = mem.get('total_page_faults', 0)
                if total_faults > 0:
                    page_faults.append({'tick': tick, 'total': total_faults})
        
        fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(14, 10))
        
        # Memory frame usage
        if ticks:
            ax1.fill_between(ticks, 0, used_frames, alpha=0.4, color='#3498DB')
            ax1.plot(ticks, used_frames, color='#2C3E50', linewidth=2)
            ax1.set_ylabel('Frames Used', fontsize=12, fontweight='bold')
            ax1.set_title('Memory Frame Usage Over Time', fontsize=14, fontweight='bold')
            ax1.grid(True, alpha=0.3)
            ax1.set_xlim(left=0)
        
        # Page faults accumulation
        if page_faults:
            fault_ticks = [pf['tick'] for pf in page_faults]
            fault_totals = [pf['total'] for pf in page_faults]
            ax2.step(fault_ticks, fault_totals, where='post', color='#E74C3C', linewidth=2)
            ax2.fill_between(fault_ticks, 0, fault_totals, step='post', alpha=0.3, color='#E74C3C')
            ax2.set_xlabel('Time (ticks)', fontsize=12, fontweight='bold')
            ax2.set_ylabel('Total Page Faults', fontsize=12, fontweight='bold')
            ax2.set_title('Cumulative Page Faults', fontsize=14, fontweight='bold')
            ax2.grid(True, alpha=0.3)
            ax2.set_xlim(left=0)
        
        plt.tight_layout()
        plt.savefig(self.output_dir / '04_memory_usage.png', dpi=300, bbox_inches='tight')
        plt.close()
        print("  ✓ Memory usage saved")
    
    def generate_page_table_heatmap(self):
        """Heatmap de tablas de páginas por proceso"""
        print("[INFO] Generating page table heatmaps...")
        
        # Get last page table for each process
        process_page_tables = {}
        
        for event in self.metrics:
            if 'page_table' in event:
                pt = event['page_table']
                proc_name = pt.get('name')
                if proc_name:
                    process_page_tables[proc_name] = pt
        
        if not process_page_tables:
            print("  ⚠ No page table data found")
            return
        
        num_processes = len(process_page_tables)
        fig, axes = plt.subplots(1, num_processes, figsize=(5*num_processes, 4))
        
        if num_processes == 1:
            axes = [axes]
        
        for idx, (proc_name, pt) in enumerate(sorted(process_page_tables.items())):
            pages = pt.get('pages', [])
            
            if not pages:
                continue
            
            # Create matrix for visualization
            num_pages = len(pages)
            data = np.zeros((num_pages, 4))  # page, frame, valid, referenced
            
            for i, page in enumerate(pages):
                data[i, 0] = page.get('page', -1)
                data[i, 1] = page.get('frame', -1)
                data[i, 2] = 1 if page.get('valid', False) else 0
                data[i, 3] = 1 if page.get('referenced', False) else 0
            
            im = axes[idx].imshow(data.T, cmap='RdYlGn', aspect='auto', vmin=-1, vmax=15)
            axes[idx].set_title(f'{proc_name} Page Table', fontweight='bold')
            axes[idx].set_yticks([0, 1, 2, 3])
            axes[idx].set_yticklabels(['Page', 'Frame', 'Valid', 'Ref'])
            axes[idx].set_xticks(range(num_pages))
            axes[idx].set_xlabel('Page Number')
            
            # Add text annotations
            for i in range(num_pages):
                for j in range(4):
                    val = int(data[i, j])
                    if j < 2:
                        text = str(val) if val >= 0 else '-'
                    else:
                        text = 'Y' if val == 1 else 'N'
                    axes[idx].text(i, j, text, ha='center', va='center',
                                  color='black', fontsize=8, fontweight='bold')
        
        plt.tight_layout()
        plt.savefig(self.output_dir / '05_page_tables.png', dpi=300, bbox_inches='tight')
        plt.close()
        print("  ✓ Page tables saved")
    
    def generate_frame_allocation(self):
        """Visualización de asignación de marcos de memoria"""
        print("[INFO] Generating memory frame allocation...")
        
        # Get final frame status
        final_frame_status = None
        for event in reversed(self.metrics):
            if 'frame_status' in event:
                final_frame_status = event['frame_status']
                break
        
        if not final_frame_status:
            print("  ⚠ No frame status data found")
            return
        
        # Count frames by process
        frame_counts = defaultdict(int)
        free_frames = 0
        
        for frame in final_frame_status:
            if frame.get('occupied', False):
                pid = frame.get('pid', -1)
                proc_name = f"P{pid}"
                frame_counts[proc_name] += 1
            else:
                free_frames += 1
        
        fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))
        
        # Pie chart
        labels = list(frame_counts.keys()) + ['Free']
        sizes = list(frame_counts.values()) + [free_frames]
        colors = [self.process_colors.get(label, '#95A5A6') for label in labels[:-1]] + ['#ECF0F1']
        
        wedges, texts, autotexts = ax1.pie(sizes, labels=labels, autopct='%1.1f%%',
                                            colors=colors, startangle=90)
        for autotext in autotexts:
            autotext.set_color('white')
            autotext.set_fontweight('bold')
        
        ax1.set_title('Memory Frame Distribution', fontsize=14, fontweight='bold')
        
        # Bar chart
        processes = sorted(frame_counts.keys())
        counts = [frame_counts[p] for p in processes]
        bars = ax2.bar(processes, counts, color=[self.process_colors.get(p, '#95A5A6') for p in processes],
                      edgecolor='black', linewidth=1.5)
        
        ax2.set_ylabel('Frames Allocated', fontsize=12, fontweight='bold')
        ax2.set_title('Frames per Process', fontsize=14, fontweight='bold')
        ax2.grid(axis='y', alpha=0.3)
        
        # Add value labels on bars
        for bar in bars:
            height = bar.get_height()
            ax2.text(bar.get_x() + bar.get_width()/2., height,
                    f'{int(height)}',
                    ha='center', va='bottom', fontweight='bold')
        
        plt.tight_layout()
        plt.savefig(self.output_dir / '06_frame_allocation.png', dpi=300, bbox_inches='tight')
        plt.close()
        print("  ✓ Frame allocation saved")
    
    def generate_io_operations(self):
        """Visualización de operaciones de E/S - mejorada con timeline"""
        print("[INFO] Generating I/O operations visualization...")
        
        # Extract I/O from state transitions
        io_operations = []
        
        for event in self.metrics:
            tick = event.get('tick', -1)
            
            # Look for I/O start (process going to WAITING state with io_request reason)
            if 'state_transitions' in event:
                for trans in event['state_transitions']:
                    if trans.get('to') == 'WAITING' and trans.get('reason') == 'io_request':
                        io_operations.append({
                            'start': tick,
                            'name': trans['name'],
                            'type': 'start'
                        })
                    elif trans.get('from') == 'WAITING' and trans.get('reason') == 'io_completed':
                        io_operations.append({
                            'end': tick,
                            'name': trans['name'],
                            'type': 'end'
                        })
        
        if not io_operations:
            print("  ⚠ No I/O operations found")
            return
        
        # Match start and end events
        io_periods = defaultdict(list)
        pending_starts = {}
        
        for op in sorted(io_operations, key=lambda x: x.get('start', x.get('end', 0))):
            name = op['name']
            if op['type'] == 'start':
                pending_starts[name] = op['start']
            elif op['type'] == 'end' and name in pending_starts:
                start = pending_starts[name]
                end = op['end']
                io_periods[name].append({'start': start, 'end': end, 'duration': end - start})
                del pending_starts[name]
        
        if not io_periods:
            print("  ⚠ No complete I/O periods found")
            return
        
        # Create visualization
        fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(14, 8), height_ratios=[2, 1])
        
        # Top: Timeline with bars showing I/O duration
        y_pos = 0
        yticks = []
        yticklabels = []
        
        all_starts = []
        all_ends = []
        
        for proc_name in sorted(io_periods.keys()):
            periods = io_periods[proc_name]
            
            for period in periods:
                start = period['start']
                end = period['end']
                duration = period['duration']
                
                all_starts.append(start)
                all_ends.append(end)
                
                # Draw I/O period as bar
                color = self.process_colors.get(proc_name, '#95A5A6')
                ax1.barh(y_pos, duration, left=start, height=0.6,
                        color=color, alpha=0.7, edgecolor='black', linewidth=1.5)
                
                # Label with duration
                mid = start + duration / 2
                ax1.text(mid, y_pos, f'{duration}', ha='center', va='center',
                        fontsize=9, fontweight='bold', color='white')
            
            yticks.append(y_pos)
            yticklabels.append(f'{proc_name} ({len(periods)} ops)')
            y_pos += 1
        
        ax1.set_yticks(yticks)
        ax1.set_yticklabels(yticklabels)
        ax1.set_xlabel('Time (ticks)', fontsize=12, fontweight='bold')
        ax1.set_ylabel('Process', fontsize=12, fontweight='bold')
        ax1.set_title('I/O Operations Timeline (Blocked Periods)', fontsize=14, fontweight='bold')
        ax1.grid(axis='x', alpha=0.3, linestyle='--')
        
        # Bottom: I/O queue size over time
        io_queue_data = []
        for event in self.metrics:
            if 'queues' in event:
                tick = event.get('tick', -1)
                blocked_io = event['queues'].get('blocked_io', [])
                io_queue_data.append({'tick': tick, 'size': len(blocked_io)})
        
        if io_queue_data:
            ticks = [d['tick'] for d in io_queue_data]
            sizes = [d['size'] for d in io_queue_data]
            ax2.step(ticks, sizes, where='post', color='#E74C3C', linewidth=2, label='I/O Queue Size')
            ax2.fill_between(ticks, 0, sizes, step='post', alpha=0.3, color='#E74C3C')
            ax2.set_xlabel('Time (ticks)', fontsize=12, fontweight='bold')
            ax2.set_ylabel('Queue Size', fontsize=12, fontweight='bold')
            ax2.set_title('Blocked I/O Queue Size', fontsize=14, fontweight='bold')
            ax2.grid(True, alpha=0.3)
            ax2.set_xlim(left=0)
        
        plt.tight_layout()
        plt.savefig(self.output_dir / '07_io_operations.png', dpi=300, bbox_inches='tight')
        plt.close()
        print("  ✓ I/O operations saved")
    
    def generate_context_switches(self):
        """Visualización de cambios de contexto"""
        print("[INFO] Generating context switches visualization...")
        
        context_switches = []
        
        for event in self.metrics:
            tick = event.get('tick', -1)
            if 'cpu' in event:
                cpu = event['cpu']
                if cpu.get('context_switch', False) and cpu.get('pid', -1) > 0:
                    context_switches.append({
                        'tick': tick,
                        'process': cpu.get('name'),
                        'event': cpu.get('event')
                    })
        
        if not context_switches:
            print("  ⚠ No context switches found")
            return
        
        fig, ax = plt.subplots(figsize=(14, 6))
        
        ticks = [cs['tick'] for cs in context_switches]
        processes = [cs['process'] for cs in context_switches]
        
        # Create color map for processes
        unique_procs = sorted(set(processes))
        proc_to_num = {p: i for i, p in enumerate(unique_procs)}
        colors = [self.process_colors.get(p, '#95A5A6') for p in processes]
        
        ax.scatter(ticks, [proc_to_num[p] for p in processes], 
                  c=colors, s=150, edgecolors='black', linewidth=1.5, zorder=3)
        
        # Connect with lines
        ax.plot(ticks, [proc_to_num[p] for p in processes], 
               color='gray', alpha=0.5, linewidth=1, linestyle='--', zorder=1)
        
        ax.set_yticks(range(len(unique_procs)))
        ax.set_yticklabels(unique_procs)
        ax.set_xlabel('Time (ticks)', fontsize=12, fontweight='bold')
        ax.set_ylabel('Process', fontsize=12, fontweight='bold')
        ax.set_title(f'Context Switches (Total: {len(context_switches)})', 
                    fontsize=14, fontweight='bold')
        ax.grid(axis='x', alpha=0.3, linestyle='--')
        
        plt.tight_layout()
        plt.savefig(self.output_dir / '08_context_switches.png', dpi=300, bbox_inches='tight')
        plt.close()
        print("  ✓ Context switches saved")
    
    def generate_summary_dashboard(self):
        """Dashboard resumen con métricas clave"""
        print("[INFO] Generating summary dashboard...")
        
        # Extract summary metrics
        total_ticks = max(e.get('tick', 0) for e in self.metrics)
        total_context_switches = sum(1 for e in self.metrics 
                                    if e.get('cpu', {}).get('context_switch', False))
        
        total_page_faults = 0
        total_replacements = 0
        for event in self.metrics:
            if 'memory' in event:
                total_page_faults = max(total_page_faults, 
                                       event['memory'].get('total_page_faults', 0))
                total_replacements = max(total_replacements,
                                        event['memory'].get('total_replacements', 0))
        
        # Count states
        state_counts = defaultdict(int)
        for event in self.metrics:
            if 'state_transitions' in event:
                for trans in event['state_transitions']:
                    state_counts[trans['to']] += 1
        
        fig = plt.figure(figsize=(16, 10))
        gs = fig.add_gridspec(3, 3, hspace=0.3, wspace=0.3)
        
        # Title
        fig.suptitle('Simulation Summary Dashboard', fontsize=16, fontweight='bold')
        
        # Metric boxes
        metrics_data = [
            ('Total Time', f'{total_ticks} ticks', '#3498DB'),
            ('Context Switches', str(total_context_switches), '#E74C3C'),
            ('Page Faults', str(total_page_faults), '#F39C12'),
            ('Page Replacements', str(total_replacements), '#9B59B6'),
            ('Processes', str(len(self.processes)), '#2ECC71'),
        ]
        
        for idx, (label, value, color) in enumerate(metrics_data):
            row, col = idx // 3, idx % 3
            ax = fig.add_subplot(gs[row, col])
            ax.text(0.5, 0.6, value, ha='center', va='center', 
                   fontsize=32, fontweight='bold', color=color)
            ax.text(0.5, 0.3, label, ha='center', va='center',
                   fontsize=14, color='gray')
            ax.set_xlim(0, 1)
            ax.set_ylim(0, 1)
            ax.axis('off')
            ax.add_patch(Rectangle((0.05, 0.1), 0.9, 0.8, fill=False, 
                                  edgecolor=color, linewidth=3))
        
        # State transitions pie - starts from row 2 to avoid overlap
        if state_counts:
            ax = fig.add_subplot(gs[2, :])
            labels = list(state_counts.keys())
            sizes = list(state_counts.values())
            pie_colors = [self.state_colors.get(l, '#95A5A6') for l in labels]
            
            wedges, texts, autotexts = ax.pie(sizes, autopct='%1.1f%%',
                                               colors=pie_colors, startangle=90)
            for autotext in autotexts:
                autotext.set_color('white')
                autotext.set_fontweight('bold')
                autotext.set_fontsize(10)
            
            # Add legend
            ax.legend(wedges, labels, title="States", loc="center left", 
                     bbox_to_anchor=(1, 0, 0.5, 1), fontsize=10)
            
            ax.set_title('State Transitions Distribution', fontsize=14, fontweight='bold')
        
        plt.savefig(self.output_dir / '09_summary_dashboard.png', dpi=300, bbox_inches='tight')
        plt.close()
        print("  ✓ Summary dashboard saved")
    
    def generate_all(self):
        """Genera todas las visualizaciones"""
        print("\n" + "="*60)
        print("  GENERATING VISUALIZATION DIAGRAMS")
        print("="*60 + "\n")
        
        self.load_metrics()
        self.clear_output_dir()
        
        self.generate_gantt_chart()
        self.generate_queue_evolution()
        self.generate_state_diagram()
        self.generate_memory_usage()
        self.generate_page_table_heatmap()
        self.generate_frame_allocation()
        self.generate_io_operations()
        self.generate_context_switches()
        self.generate_summary_dashboard()
        
        print("\n" + "="*60)
        print(f"  ✓ All diagrams generated in: {self.output_dir}")
        print("="*60 + "\n")


def main():
    import sys
    
    # Default paths
    metrics_file = 'data/resultados/metrics.jsonl'
    output_dir = 'data/diagramas'
    
    # Allow override from command line
    if len(sys.argv) > 1:
        metrics_file = sys.argv[1]
    if len(sys.argv) > 2:
        output_dir = sys.argv[2]
    
    visualizer = MetricsVisualizer(metrics_file, output_dir)
    visualizer.generate_all()


if __name__ == '__main__':
    main()
