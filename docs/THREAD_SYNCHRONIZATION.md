# Sincronización de Hilos en el Simulador de Sistema Operativo

## 1. Introducción

Este documento explica en detalle los mecanismos de sincronización de hilos implementados en el simulador de sistema operativo. El simulador utiliza hilos para simular la ejecución concurrente de procesos, y emplea varios mecanismos de sincronización para coordinar la interacción entre el planificador de CPU, la gestión de memoria y los dispositivos de E/S.

## 2. Componentes Principales de Sincronización

### 2.1 Estructura del Proceso (`Process`)

Cada proceso en el simulador tiene los siguientes componentes de sincronización:

```cpp
// En include/core/process.hpp
std::unique_ptr<std::thread> process_thread;    // Hilo asociado al proceso
mutable std::mutex process_mutex;                // Mutex para sincronización
mutable std::condition_variable state_cv;        // Variable de condición para cambios de estado
std::atomic<ProcessState> state;                 // Estado atómico del proceso
std::atomic<bool> should_terminate;              // Señal para terminar el hilo
std::atomic<bool> step_complete;                 // Indica si el paso de ejecución está completo
```

#### Propósito de cada componente:

| Componente | Propósito |
|------------|-----------|
| `process_thread` | Ejecuta la lógica del proceso de forma concurrente |
| `process_mutex` | Protege el acceso a los datos del proceso |
| `state_cv` | Permite que el hilo espere cambios de estado eficientemente |
| `state` | Estado atómico que puede leerse/escribirse de forma segura |
| `should_terminate` | Señal atómica para solicitar la terminación del hilo |
| `step_complete` | Indica atómicamente que el paso de ejecución terminó |

### 2.2 Planificador de CPU (`CPUScheduler`)

El planificador de CPU coordina la ejecución de todos los procesos:

```cpp
// En include/cpu/cpu_scheduler.hpp
std::mutex scheduler_mutex;           // Protege el acceso al planificador
std::atomic<bool> simulation_running; // Indica si la simulación está activa
```

### 2.3 Gestor de Memoria (`MemoryManager`)

El gestor de memoria maneja las páginas y marcos:

```cpp
// En include/memory/memory_manager.hpp
std::mutex mutex_;  // Protege todas las operaciones de memoria
```

### 2.4 Gestor de E/S (`IOManager`)

El gestor de E/S coordina los dispositivos:

```cpp
// En include/io/io_manager.hpp
mutable std::mutex manager_mutex;  // Protege el acceso a los dispositivos
```

## 3. Flujo de Ejecución de un Proceso

### 3.1 Inicio del Hilo del Proceso

Cuando se carga un proceso, el planificador inicia su hilo:

```cpp
// En src/core/process.cpp
void Process::start_thread() {
  stop_thread();                    // Detiene cualquier hilo previo
  should_terminate = false;         // Reinicia la señal de terminación
  step_complete = false;            // Reinicia el estado del paso

  process_thread = std::make_unique<std::thread>(&Process::thread_function, this);
}
```

### 3.2 Función Principal del Hilo

El hilo del proceso ejecuta un bucle que espera ser notificado:

```cpp
// En src/core/process.cpp
void Process::thread_function() {
  while (!should_terminate.load()) {
    std::unique_lock<std::mutex> lock(process_mutex);

    // Espera hasta que:
    // 1. El proceso esté en estado RUNNING y no haya completado el paso, O
    // 2. Se haya solicitado la terminación
    state_cv.wait(lock, [this]() {
      return (state.load() == ProcessState::RUNNING && !step_complete.load()) ||
             should_terminate.load();
    });

    if (should_terminate.load())
      break;

    // Simula trabajo del proceso
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Marca el paso como completo y notifica
    step_complete.store(true);
    state_cv.notify_all();
  }
}
```

### 3.3 Coordinación con el Planificador

El planificador coordina la ejecución siguiendo este patrón:

```cpp
// En src/cpu/cpu_scheduler.cpp

// 1. Notifica al proceso que puede ejecutar
void CPUScheduler::notify_process_running(std::shared_ptr<Process> proc) {
  if (!proc) return;
  std::lock_guard<std::mutex> lock(proc->process_mutex);

  ProcessState old_state = proc->state.load();
  proc->state = ProcessState::RUNNING;  // Cambia estado a RUNNING
  proc->step_complete = false;           // Reinicia el indicador
  proc->state_cv.notify_all();           // Notifica al hilo del proceso
}

// 2. Espera a que el proceso complete su paso
void CPUScheduler::wait_for_process_step(std::shared_ptr<Process> proc) {
  if (!proc) return;
  std::unique_lock<std::mutex> lock(proc->process_mutex);
  
  // Espera con timeout de 1 segundo
  bool step_done = proc->state_cv.wait_for(
    lock, 
    std::chrono::milliseconds(1000),
    [&proc]() { return proc->step_complete.load(); }
  );

  if (!step_done) {
    // Timeout: el proceso no completó a tiempo
    std::cout << "Warning: Process " << proc->pid << " did not complete step in time." << std::endl;
    proc->should_terminate = true;
    proc->state_cv.notify_all();
    return;
  }

  proc->step_complete = false;  // Reinicia para el próximo paso
}
```

### 3.4 Terminación del Hilo

Cuando un proceso termina, su hilo se detiene de forma segura:

```cpp
// En src/core/process.cpp
void Process::stop_thread() {
  if (!process_thread) return;

  {
    std::lock_guard<std::mutex> lock(process_mutex);
    should_terminate = true;      // Señala terminación
    state_cv.notify_all();         // Despierta al hilo si está esperando
  }

  if (process_thread->joinable()) {
    process_thread->join();        // Espera a que el hilo termine
  }

  process_thread.reset();          // Libera el recurso
}
```

## 4. Sincronización con la Gestión de Memoria

### 4.1 Preparación de Memoria para CPU

Antes de ejecutar un proceso, se verifica que sus páginas estén en memoria:

```cpp
// En src/memory/memory_manager.cpp
bool MemoryManager::prepare_process_for_cpu(std::shared_ptr<Process> process, int current_time) {
  if (!process) return false;
  std::lock_guard<std::mutex> lock(mutex_);  // Bloquea el acceso a memoria

  // Verifica si todas las páginas están en memoria
  if (are_all_pages_resident(*process)) {
    set_process_pages_referenced(*process, true);
    processes_waiting_on_memory.erase(process->pid);
    return true;  // Proceso listo para CPU
  }

  // Encola las páginas faltantes para carga
  std::vector<int> missing_pages;
  // ... (identificación de páginas faltantes)
  
  if (!missing_pages.empty()) {
    enqueue_missing_pages(process, missing_pages, current_time);
  }

  processes_waiting_on_memory.insert(process->pid);
  return false;  // Proceso debe esperar (MEMORY_WAITING)
}
```

### 4.2 Callback de Memoria Lista

Cuando las páginas de un proceso se cargan, se notifica al planificador:

```cpp
// Configuración del callback en cpu_scheduler.cpp
void CPUScheduler::set_memory_manager(std::shared_ptr<MemoryManager> mm) {
  std::lock_guard<std::mutex> lock(scheduler_mutex);
  memory_manager = mm;
  if (memory_manager) {
    memory_manager->set_ready_callback([this](std::shared_ptr<Process> proc) {
      this->handle_memory_ready(proc);
    });
  }
}

// Manejo cuando el proceso está listo
void CPUScheduler::handle_memory_ready(std::shared_ptr<Process> proc) {
  if (!proc) return;
  std::lock_guard<std::mutex> lock(scheduler_mutex);  // Protege el planificador
  
  if (!scheduler || proc->state == ProcessState::TERMINATED)
    return;

  ProcessState old_state = proc->state.load();
  proc->state = ProcessState::READY;           // Cambia a READY
  scheduler->remove_process(proc->pid);
  scheduler->add_process(proc);                // Re-agrega a la cola
  request_preemption_if_needed(proc);
}
```

## 5. Sincronización con E/S

### 5.1 Envío de Solicitudes de E/S

Cuando un proceso necesita E/S:

```cpp
// En cpu_scheduler.cpp - execute_step()
auto *current_burst = running_process->get_current_burst_mutable();
if (current_burst && current_burst->type == BurstType::IO && io_manager) {
  if (memory_manager) {
    memory_manager->mark_process_inactive(*running_process);
  }
  
  ProcessState old_state = running_process->state.load();
  running_process->state = ProcessState::WAITING;  // Bloquea por E/S
  scheduler->remove_process(running_process->pid);

  // Crea y envía la solicitud de E/S
  auto request = std::make_shared<IORequest>(running_process, *current_burst, current_time);
  io_manager->submit_io_request(request);

  running_process = nullptr;
}
```

### 5.2 Callback de E/S Completada

```cpp
// Configuración del callback
void CPUScheduler::set_io_manager(std::shared_ptr<IOManager> manager) {
  std::lock_guard<std::mutex> lock(scheduler_mutex);
  io_manager = manager;
  if (io_manager) {
    io_manager->set_completion_callback(
      [this](std::shared_ptr<Process> proc, int completion_time) {
        handle_io_completion(proc, completion_time);
      }
    );
  }
}

// Manejo cuando E/S completa
void CPUScheduler::handle_io_completion(std::shared_ptr<Process> proc, int completion_time) {
  if (!proc) return;
  std::lock_guard<std::mutex> lock(scheduler_mutex);

  // Avanza a la siguiente ráfaga
  auto *burst = proc->get_current_burst_mutable();
  if (burst && burst->type == BurstType::IO) {
    burst->remaining_time = 0;
  }
  proc->advance_to_next_burst();

  if (proc->is_completed()) {
    // Proceso terminó
    proc->state = ProcessState::TERMINATED;
    completed_processes.push_back(proc);
  } else {
    // Proceso vuelve a la cola de listos
    proc->state = ProcessState::READY;
    if (scheduler) {
      scheduler->add_process(proc);
      request_preemption_if_needed(proc);
    }
  }
}
```

## 6. Avance del Tiempo y Coordinación

### 6.1 Avance de Dispositivos de E/S

El planificador coordina el avance de los dispositivos:

```cpp
void CPUScheduler::advance_io_devices(int time_slice, int step_start_time,
                                      std::unique_lock<std::mutex> &lock) {
  if (!io_manager || time_slice <= 0) return;

  lock.unlock();  // Libera el lock del scheduler temporalmente
  io_manager->execute_all_devices(time_slice, step_start_time);
  lock.lock();    // Vuelve a adquirir el lock
}
```

### 6.2 Avance de la Cola de Fallos de Página

```cpp
void CPUScheduler::advance_memory_manager(int time_slice, int step_start_time,
                                          std::unique_lock<std::mutex> &lock) {
  if (!memory_manager || time_slice <= 0) return;

  lock.unlock();  // Libera el lock temporalmente
  memory_manager->advance_fault_queue(time_slice, step_start_time);
  lock.lock();    // Vuelve a adquirir el lock
}
```

## 7. Diagrama de Secuencia

```
┌──────────┐     ┌────────────────┐     ┌───────────────┐     ┌───────────┐
│Scheduler │     │Process Thread  │     │MemoryManager  │     │ IOManager │
└────┬─────┘     └───────┬────────┘     └───────┬───────┘     └─────┬─────┘
     │                   │                      │                   │
     │ notify_running()  │                      │                   │
     │──────────────────>│                      │                   │
     │                   │                      │                   │
     │ state = RUNNING   │                      │                   │
     │ notify_all()      │                      │                   │
     │                   │                      │                   │
     │                   │ (despierta)          │                   │
     │                   │ sleep(10ms)          │                   │
     │                   │ step_complete=true   │                   │
     │                   │ notify_all()         │                   │
     │                   │                      │                   │
     │ wait_for_step()   │                      │                   │
     │<──────────────────│                      │                   │
     │                   │                      │                   │
     │ execute(quantum)  │                      │                   │
     │──────────────────>│                      │                   │
     │                   │                      │                   │
     │         [Si necesita memoria]            │                   │
     │────────────────────────────────────────>│                   │
     │ prepare_process_for_cpu()               │                   │
     │                                          │                   │
     │ [Si falta página]                        │                   │
     │<────────────────────────────────────────│                   │
     │ return false (MEMORY_WAITING)           │                   │
     │                   │                      │                   │
     │         [Cuando página cargada]          │                   │
     │<────────────────────────────────────────│                   │
     │ handle_memory_ready() via callback      │                   │
     │                   │                      │                   │
     │         [Si necesita E/S]                │                   │
     │──────────────────────────────────────────────────────────>│
     │ submit_io_request()                      │                   │
     │                   │                      │                   │
     │         [Cuando E/S completa]            │                   │
     │<──────────────────────────────────────────────────────────│
     │ handle_io_completion() via callback     │                   │
     │                   │                      │                   │
```

## 8. Estados del Proceso y Transiciones

```
         ┌──────────────────────────────────────────────────────────┐
         │                                                          │
         v                                                          │
    ┌────────┐    allocate     ┌───────┐    schedule     ┌─────────┐
    │  NEW   │ ─────────────> │ READY │ ─────────────> │ RUNNING │
    └────────┘                 └───────┘                 └─────────┘
                                  ^                          │
                                  │                          │
                    ┌─────────────┴──────────────┐           │
                    │                            │           │
               io_complete              memory_loaded       │
                    │                            │           │
              ┌───────────┐              ┌───────────────┐   │
              │  WAITING  │              │MEMORY_WAITING │   │
              └───────────┘              └───────────────┘   │
                    ^                            ^           │
                    │                            │           │
               io_request                  page_fault        │
                    │                            │           │
                    └────────────────────────────┴───────────┘
                                                 │
                                       burst_completed
                                                 │
                                                 v
                                          ┌────────────┐
                                          │ TERMINATED │
                                          └────────────┘
```

## 9. Patrones de Sincronización Utilizados

### 9.1 Patrón Monitor

El `CPUScheduler` actúa como un monitor que coordina el acceso a recursos compartidos:

```cpp
void CPUScheduler::execute_step(int quantum) {
  std::unique_lock<std::mutex> scheduler_lock(scheduler_mutex);
  // ... operaciones protegidas
}
```

### 9.2 Patrón Productor-Consumidor

La cola de fallos de página utiliza este patrón:

- **Productor**: `prepare_process_for_cpu()` encola páginas faltantes
- **Consumidor**: `advance_fault_queue()` procesa las cargas

### 9.3 Patrón Callback/Observer

Los callbacks permiten notificación asíncrona:

```cpp
memory_manager->set_ready_callback([this](std::shared_ptr<Process> proc) {
  this->handle_memory_ready(proc);
});

io_manager->set_completion_callback(
  [this](std::shared_ptr<Process> proc, int completion_time) {
    handle_io_completion(proc, completion_time);
  }
);
```

## 10. Consideraciones de Seguridad de Hilos

### 10.1 Prevención de Interbloqueos (Deadlocks)

1. **Orden de bloqueo consistente**: Se libera el `scheduler_mutex` antes de llamar a operaciones de memoria/E/S
2. **Timeout en esperas**: `wait_for_process_step()` usa `wait_for()` con timeout

### 10.2 Prevención de Condiciones de Carrera

1. **Variables atómicas**: `state`, `should_terminate`, `step_complete` son `std::atomic`
2. **Mutex por componente**: Cada componente tiene su propio mutex
3. **Lock guards**: Se usan `std::lock_guard` y `std::unique_lock`

### 10.3 Liberación Temporal de Locks

Para evitar inversión de prioridades y permitir progreso:

```cpp
void CPUScheduler::advance_io_devices(..., std::unique_lock<std::mutex> &lock) {
  lock.unlock();                    // Permite que otros hilos progresen
  io_manager->execute_all_devices(...);
  lock.lock();                      // Re-adquiere el lock
}
```

## 11. Resumen

| Mecanismo | Uso en el Simulador |
|-----------|---------------------|
| `std::mutex` | Protección de secciones críticas |
| `std::condition_variable` | Espera eficiente de eventos |
| `std::atomic` | Variables de estado thread-safe |
| `std::thread` | Ejecución concurrente de procesos |
| Callbacks | Notificación asíncrona de eventos |

La sincronización en este simulador sigue buenas prácticas de programación concurrente, utilizando primitivas de C++11/14 para garantizar la correcta coordinación entre el planificador de CPU, la gestión de memoria y los dispositivos de E/S.
