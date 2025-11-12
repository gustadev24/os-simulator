#ifndef CPU_SCHEDULER_HPP
#define CPU_SCHEDULER_HPP

#include "core/process.hpp"
#include "cpu/scheduler.hpp"
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace OSSimulator {

class CPUScheduler {
private:
    std::unique_ptr<Scheduler> scheduler;
    
    std::vector<std::shared_ptr<Process>> all_processes;
    std::vector<std::shared_ptr<Process>> completed_processes;

    int current_time;
    std::shared_ptr<Process> running_process;

    int context_switches;

    using MemoryCheckCallback = std::function<bool(const Process &)>;
    MemoryCheckCallback memory_check_callback;

    std::mutex scheduler_mutex;
    std::atomic<bool> simulation_running;

    void spawn_process_thread(std::shared_ptr<Process> proc);
    void notify_process_running(std::shared_ptr<Process> proc);
    void wait_for_process_step(std::shared_ptr<Process> proc);
    void terminate_all_threads();

public:
    CPUScheduler();
    ~CPUScheduler();

    void set_scheduler(std::unique_ptr<Scheduler> sched);
    void set_memory_callback(MemoryCheckCallback callback);

    void add_process(std::shared_ptr<Process> process);

    void load_processes(const std::vector<std::shared_ptr<Process>> &processes);

    bool check_and_allocate_memory(Process &process);
    void add_arrived_processes();
    void execute_step(int quantum = 1);
    void run_until_completion();
    bool has_pending_processes() const;
    int get_current_time() const;
    int get_context_switches() const;

    const std::vector<std::shared_ptr<Process>> &get_completed_processes() const;
    const std::vector<std::shared_ptr<Process>> &get_all_processes() const;

    double get_average_waiting_time() const;
    double get_average_turnaround_time() const;
    double get_average_response_time() const;

    void reset();
};

} // namespace OSSimulator

#endif // CPU_SCHEDULER_HPP
