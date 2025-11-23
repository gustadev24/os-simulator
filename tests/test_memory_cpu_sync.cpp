#include <catch2/catch_test_macros.hpp>
#include "core/process.hpp"
#include "cpu/cpu_scheduler.hpp"
#include "cpu/fcfs_scheduler.hpp"
#include "io/io_device.hpp"
#include "io/io_fcfs_scheduler.hpp"
#include "io/io_manager.hpp"
#include "memory/fifo_replacement.hpp"
#include "memory/memory_manager.hpp"
#include <algorithm>

using namespace OSSimulator;

namespace {

std::shared_ptr<IOManager> build_default_io_manager() {
    auto io_manager = std::make_shared<IOManager>();
    auto disk = std::make_shared<IODevice>("disk");
    disk->set_scheduler(std::make_unique<IOFCFSScheduler>());
    io_manager->add_device("disk", disk);
    return io_manager;
}

}

TEST_CASE("Full system integration - single process", "[integration]") {
    CPUScheduler cpu_scheduler;
    cpu_scheduler.set_scheduler(std::make_unique<FCFSScheduler>());

    auto memory_manager = std::make_shared<MemoryManager>(2, std::make_unique<FIFOReplacement>());
    cpu_scheduler.set_memory_manager(memory_manager);

    auto io_manager = build_default_io_manager();
    cpu_scheduler.set_io_manager(io_manager);

    auto process = std::make_shared<Process>(
        1, "P1", 0,
        std::vector<Burst>{Burst(BurstType::CPU, 2),
                           Burst(BurstType::IO, 2, "disk"),
                           Burst(BurstType::CPU, 1)},
        1, 2);
    process->memory_access_trace = {0, 1};

    cpu_scheduler.load_processes({process});
    cpu_scheduler.run_until_completion();

    REQUIRE(process->state == ProcessState::TERMINATED);
    REQUIRE(cpu_scheduler.get_completed_processes().size() == 1);
    REQUIRE(memory_manager->get_total_page_faults() == 2);
    REQUIRE(memory_manager->get_total_replacements() == 0);
    REQUIRE_FALSE(io_manager->has_pending_io());
    REQUIRE(cpu_scheduler.get_current_time() == 7);
}

TEST_CASE("Full system integration - multiple processes", "[integration]") {
    CPUScheduler cpu_scheduler;
    cpu_scheduler.set_scheduler(std::make_unique<FCFSScheduler>());

    auto memory_manager = std::make_shared<MemoryManager>(5, std::make_unique<FIFOReplacement>());
    cpu_scheduler.set_memory_manager(memory_manager);

    auto io_manager = build_default_io_manager();
    cpu_scheduler.set_io_manager(io_manager);

    auto process1 = std::make_shared<Process>(
        1, "P1", 0,
        std::vector<Burst>{Burst(BurstType::CPU, 2),
                           Burst(BurstType::IO, 2, "disk"),
                           Burst(BurstType::CPU, 1)},
        1, 2);
    process1->memory_access_trace = {0, 1};

    auto process2 = std::make_shared<Process>(
        2, "P2", 1,
        std::vector<Burst>{Burst(BurstType::CPU, 1),
                           Burst(BurstType::IO, 1, "disk"),
                           Burst(BurstType::CPU, 2)},
        2, 3);
    process2->memory_access_trace = {2};

    cpu_scheduler.load_processes({process1, process2});
    cpu_scheduler.run_until_completion();

    const auto &completed = cpu_scheduler.get_completed_processes();
    REQUIRE(completed.size() == 2);
    REQUIRE(std::all_of(completed.begin(), completed.end(), [](const auto &proc) {
        return proc->state == ProcessState::TERMINATED;
    }));

    REQUIRE(memory_manager->get_total_page_faults() == 5);
    REQUIRE(memory_manager->get_total_replacements() == 0);
    REQUIRE_FALSE(io_manager->has_pending_io());
    REQUIRE(cpu_scheduler.get_current_time() == 9);
}
