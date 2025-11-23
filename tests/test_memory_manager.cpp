#include "core/process.hpp"
#include "memory/fifo_replacement.hpp"
#include "memory/memory_manager.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace OSSimulator;

TEST_CASE("MemoryManager Initialization", "[memory]") {
  auto algo = std::make_unique<FIFOReplacement>();
  MemoryManager mm(4, std::move(algo));

  REQUIRE(mm.get_total_page_faults() == 0);
  REQUIRE(mm.get_total_replacements() == 0);
}

TEST_CASE("Process waits until all pages are loaded", "[memory]") {
  auto algo = std::make_unique<FIFOReplacement>();
  MemoryManager mm(2, std::move(algo), 1);

  bool callback_called = false;
  mm.set_ready_callback([&](std::shared_ptr<Process> proc) {
    REQUIRE(proc);
    callback_called = true;
    proc->state = ProcessState::READY;
  });

  auto proc = std::make_shared<Process>(1, "P1", 0, 5, 0, 2);
  mm.allocate_initial_memory(*proc);
  mm.register_process(proc);

  REQUIRE_FALSE(mm.prepare_process_for_cpu(proc, 0));
  REQUIRE(proc->page_faults == 2);
  REQUIRE(mm.get_total_page_faults() == 2);

  mm.advance_fault_queue(1, 0);
  REQUIRE(proc->active_pages_count == 1);
  REQUIRE_FALSE(callback_called);

  mm.advance_fault_queue(1, 1);
  REQUIRE(proc->active_pages_count == 2);
  REQUIRE(callback_called);
  REQUIRE(mm.prepare_process_for_cpu(proc, 2));
}

TEST_CASE("Referenced pages block eviction until released", "[memory]") {
  auto algo = std::make_unique<FIFOReplacement>();
  MemoryManager mm(2, std::move(algo), 1);

  auto procA = std::make_shared<Process>(1, "A", 0, 5, 0, 2);
  auto procB = std::make_shared<Process>(2, "B", 0, 5, 0, 1);

  mm.allocate_initial_memory(*procA);
  mm.allocate_initial_memory(*procB);
  mm.register_process(procA);
  mm.register_process(procB);

  REQUIRE_FALSE(mm.prepare_process_for_cpu(procA, 0));
  mm.advance_fault_queue(2, 0);
  REQUIRE(mm.prepare_process_for_cpu(procA, 2));
  REQUIRE(procA->active_pages_count == 2);

  REQUIRE_FALSE(mm.prepare_process_for_cpu(procB, 3));
  mm.advance_fault_queue(1, 3);
  REQUIRE(procB->active_pages_count == 0);

  mm.mark_process_inactive(*procA);
  mm.advance_fault_queue(1, 4);

  REQUIRE(procB->active_pages_count == 1);
  REQUIRE(mm.get_total_replacements() == 1);
}
