#include "cpu/cpu_scheduler.hpp"
#include "cpu/round_robin_scheduler.hpp"
#include "io/io_manager.hpp"
#include "io/io_request.hpp"
#include <chrono>
#include <iostream>

namespace OSSimulator {

CPUScheduler::CPUScheduler()
    : scheduler(nullptr), current_time(0), running_process(nullptr),
      context_switches(0), memory_check_callback(nullptr),
      simulation_running(false) {
        // all_processes.reserve(100);
      }

CPUScheduler::~CPUScheduler() {
  terminate_all_threads();
}

void CPUScheduler::set_scheduler(std::unique_ptr<Scheduler> sched) {
  scheduler = std::move(sched);
}

void CPUScheduler::set_memory_manager(std::shared_ptr<MemoryManager> mm) {
        std::lock_guard<std::mutex> lock(scheduler_mutex);
        memory_manager = mm;
        if (memory_manager) {
                memory_manager->set_ready_callback(
                        [this](std::shared_ptr<Process> proc) { this->handle_memory_ready(proc); });
        }
}

void CPUScheduler::set_memory_callback(MemoryCheckCallback callback) {
  memory_check_callback = callback;
}

void CPUScheduler::set_io_manager(std::shared_ptr<IOManager> manager) {
    std::lock_guard<std::mutex> lock(scheduler_mutex);
    io_manager = manager;
    if (io_manager) {
        io_manager->set_completion_callback(
                [this](std::shared_ptr<Process> proc, int completion_time) {
                    handle_io_completion(proc, completion_time);
                });
    }
}

void CPUScheduler::add_process(std::shared_ptr<Process> process) {
    all_processes.push_back(process);
    spawn_process_thread(process);
}

void CPUScheduler::load_processes(const std::vector<std::shared_ptr<Process>> &processes) {
    all_processes = processes;
    for (auto &proc : all_processes) {
        spawn_process_thread(proc);
    }

    completed_processes.clear();
    current_time = 0;
    context_switches = 0;
    running_process = nullptr;

    if (scheduler) scheduler->clear();
}
bool CPUScheduler::check_and_allocate_memory(Process &process) {
  if (memory_check_callback)
    return memory_check_callback(process);
  process.memory_allocated = true;
  return true;
}

void CPUScheduler::add_arrived_processes() {
    for (auto &proc : all_processes) {
        if (proc->state == ProcessState::NEW && proc->has_arrived(current_time)) {
            bool allocated = false;
            if (memory_manager) {
                if (memory_manager->allocate_initial_memory(*proc)) {
                    memory_manager->register_process(proc);
                    allocated = true;
                    proc->memory_allocated = true;
                }
            } else {
                allocated = check_and_allocate_memory(*proc);
            }

            if (allocated) {
                proc->state = ProcessState::READY;
                scheduler->add_process(proc);
            }
        }
    }
}

void CPUScheduler::execute_step(int quantum) {
    std::unique_lock<std::mutex> scheduler_lock(scheduler_mutex);

    add_arrived_processes();

    if (!scheduler->has_processes()) {
        if (has_pending_processes()) {
            int idle_start = current_time;
            advance_memory_manager(1, idle_start, scheduler_lock);
            advance_io_devices(1, idle_start, scheduler_lock);
            current_time++;
        }
        return;
    }

    auto next = scheduler->get_next_process();
    while (next &&
           (next->state == ProcessState::WAITING ||
            next->state == ProcessState::MEMORY_WAITING ||
            next->state == ProcessState::TERMINATED)) {
        scheduler->remove_process(next->pid);
        next = scheduler->has_processes() ? scheduler->get_next_process() : nullptr;
    }

    if (!next) {
        if (has_pending_processes()) {
            int idle_start = current_time;
            advance_io_devices(1, idle_start, scheduler_lock);
            current_time++;
        }
        return;
    }

    if (!running_process || running_process->pid != next->pid)
        context_switches++;

    running_process = next;

    if (memory_manager) {
        if (!memory_manager->prepare_process_for_cpu(running_process, current_time)) {
            running_process->state = ProcessState::MEMORY_WAITING;
            scheduler->remove_process(running_process->pid);
            running_process = nullptr;
            return;
        }
    }

    auto *current_burst = running_process->get_current_burst_mutable();
    if (current_burst && current_burst->type == BurstType::IO && io_manager) {
        if (memory_manager) {
            memory_manager->mark_process_inactive(*running_process);
        }
        running_process->state = ProcessState::WAITING;
        scheduler->remove_process(running_process->pid);

        auto request = std::make_shared<IORequest>(running_process, *current_burst, current_time);
        io_manager->submit_io_request(request);

        running_process = nullptr;
        return;
    }

    notify_process_running(running_process);
    wait_for_process_step(running_process);

    int step_start_time = current_time;
    int time_executed = running_process->execute(quantum, current_time);
    current_time += time_executed;

    advance_memory_manager(time_executed, step_start_time, scheduler_lock);
    advance_io_devices(time_executed, step_start_time, scheduler_lock);

    if (running_process->is_completed()) {
        running_process->calculate_metrics();
        running_process->stop_thread();

        completed_processes.push_back(running_process);

        running_process->state = ProcessState::TERMINATED;

        scheduler->remove_process(running_process->pid);

        if (memory_manager) {
            memory_manager->mark_process_inactive(*running_process);
            memory_manager->release_process_memory(running_process->pid);
        }

        pending_preemption = false;
        running_process = nullptr;
    } else {
        if (pending_preemption &&
            (scheduler->get_algorithm() == SchedulingAlgorithm::ROUND_ROBIN ||
             scheduler->get_algorithm() == SchedulingAlgorithm::PRIORITY)) {
            pending_preemption = false;
            if (memory_manager) {
                memory_manager->mark_process_inactive(*running_process);
            }
            {
                std::lock_guard<std::mutex> lock(running_process->process_mutex);
                running_process->state = ProcessState::READY;
                running_process->state_cv.notify_all();
            }
            scheduler->remove_process(running_process->pid);
            scheduler->add_process(running_process);
            running_process = nullptr;
            return;
        }

        if (pending_preemption) {
            pending_preemption = false;
        }

        if (scheduler->get_algorithm() == SchedulingAlgorithm::ROUND_ROBIN) {
            if (memory_manager) {
                memory_manager->mark_process_inactive(*running_process);
            }
            {
                std::lock_guard<std::mutex> lock(running_process->process_mutex);
                running_process->state = ProcessState::READY;
                running_process->step_complete = false;
                running_process->state_cv.notify_all();
            }
            scheduler->remove_process(running_process->pid);
            scheduler->add_process(running_process);
        } else {
            std::lock_guard<std::mutex> lock(running_process->process_mutex);
            running_process->state = ProcessState::READY;
            running_process->step_complete = false;
            running_process->state_cv.notify_all();
        }
    }
}

void CPUScheduler::run_until_completion() {
  simulation_running = true;
  while (simulation_running && (has_pending_processes() || scheduler->has_processes())) {
    int quantum = 0;
        if (scheduler->get_algorithm() == SchedulingAlgorithm::ROUND_ROBIN) {
            if (auto *rr = dynamic_cast<RoundRobinScheduler *>(scheduler.get()))
                quantum = rr->get_quantum();
        } else if (scheduler->get_algorithm() == SchedulingAlgorithm::PRIORITY) {
                quantum = 1;
        }
    execute_step(quantum);
  }
}

bool CPUScheduler::has_pending_processes() const {
    for (const auto &proc : all_processes)
        if (proc->state != ProcessState::TERMINATED)
            return true;
    return false;
}

int CPUScheduler::get_current_time() const { return current_time; }
int CPUScheduler::get_context_switches() const { return context_switches; }

const std::vector<std::shared_ptr<Process>>& CPUScheduler::get_completed_processes() const {
    return completed_processes;
}

const std::vector<std::shared_ptr<Process>>& CPUScheduler::get_all_processes() const {
    return all_processes;
}

double CPUScheduler::get_average_waiting_time() const {
    if (completed_processes.empty()) return 0.0;
    int total = 0;
    for (const auto &proc : completed_processes)
        total += proc->waiting_time;
    return static_cast<double>(total) / completed_processes.size();
}

double CPUScheduler::get_average_turnaround_time() const {
    if (completed_processes.empty()) return 0.0;
    int total = 0;
    for (const auto &proc : completed_processes)
        total += proc->turnaround_time;
    return static_cast<double>(total) / completed_processes.size();
}

double CPUScheduler::get_average_response_time() const {
    if (completed_processes.empty()) return 0.0;
    int total = 0;
    for (const auto &proc : completed_processes)
        total += proc->response_time;
    return static_cast<double>(total) / completed_processes.size();
}

void CPUScheduler::reset() {
  terminate_all_threads();
  for (auto &proc : all_processes) proc->reset();
  completed_processes.clear();
  current_time = 0;
  context_switches = 0;
  running_process = nullptr;
  if (scheduler) scheduler->clear();
}

void CPUScheduler::spawn_process_thread(std::shared_ptr<Process> proc) {
    if (!proc->is_thread_running()) proc->start_thread();
}

void CPUScheduler::notify_process_running(std::shared_ptr<Process> proc) {
    if (!proc) return;
    std::lock_guard<std::mutex> lock(proc->process_mutex);

    proc->state = ProcessState::RUNNING;
    proc->step_complete = false;
    proc->state_cv.notify_all();
}

void CPUScheduler::wait_for_process_step(std::shared_ptr<Process> proc) {
    if (!proc) return;
    std::unique_lock<std::mutex> lock(proc->process_mutex);
    bool step_done = proc->state_cv.wait_for(
        lock, std::chrono::milliseconds(1000),
        [&proc]() { return proc->step_complete.load(); });

    if (!step_done) {
        std::cout << "Warning: Process " << proc->pid
                  << " did not complete step in time. Terminating thread."
                  << std::endl;
        proc->should_terminate = true;
        proc->state_cv.notify_all();
        return;
    }

    proc->step_complete = false;
}

void CPUScheduler::handle_io_completion(std::shared_ptr<Process> proc,
                                        int completion_time) {
    if (!proc) return;

    std::lock_guard<std::mutex> lock(scheduler_mutex);

    auto *burst = proc->get_current_burst_mutable();
    if (burst && burst->type == BurstType::IO) {
        burst->remaining_time = 0;
    }

    proc->advance_to_next_burst();

    if (proc->is_completed()) {
        proc->completion_time = completion_time;
        proc->calculate_metrics();
        proc->stop_thread();
        proc->state = ProcessState::TERMINATED;
        completed_processes.push_back(proc);
    } else {
        proc->state = ProcessState::READY;
        if (scheduler) {
            scheduler->add_process(proc);
            request_preemption_if_needed(proc);
        }
    }
}

void CPUScheduler::handle_memory_ready(std::shared_ptr<Process> proc) {
    if (!proc) return;

    std::lock_guard<std::mutex> lock(scheduler_mutex);
    if (!scheduler || proc->state == ProcessState::TERMINATED) return;

    proc->state = ProcessState::READY;
    scheduler->remove_process(proc->pid);
    scheduler->add_process(proc);
    request_preemption_if_needed(proc);
}

void CPUScheduler::advance_memory_manager(int time_slice, int step_start_time,
                                          std::unique_lock<std::mutex> &lock) {
    if (!memory_manager || time_slice <= 0) return;

    lock.unlock();
    memory_manager->advance_fault_queue(time_slice, step_start_time);
    lock.lock();
}

void CPUScheduler::advance_io_devices(int time_slice, int step_start_time,
                                      std::unique_lock<std::mutex> &lock) {
    if (!io_manager || time_slice <= 0) return;

    lock.unlock();
    io_manager->execute_all_devices(time_slice, step_start_time);
    lock.lock();
}

void CPUScheduler::request_preemption_if_needed(std::shared_ptr<Process> proc) {
    if (!scheduler || !proc || !running_process) return;

    switch (scheduler->get_algorithm()) {
    case SchedulingAlgorithm::ROUND_ROBIN:
        pending_preemption = true;
        break;
    case SchedulingAlgorithm::PRIORITY:
        if (should_preempt_priority(proc)) {
            pending_preemption = true;
        }
        break;
    default:
        break;
    }
}

bool CPUScheduler::should_preempt_priority(std::shared_ptr<Process> candidate) const {
    if (!candidate || !running_process) return false;
    return candidate->priority < running_process->priority;
}

void CPUScheduler::terminate_all_threads() {
    simulation_running = false;
    for (auto &proc : all_processes) {
        try {
            proc->stop_thread();
        } catch (...) {}
    }
}


} // namespace OSSimulator
