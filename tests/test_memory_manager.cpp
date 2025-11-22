#include <catch2/catch_test_macros.hpp>
#include "memory/memory_manager.hpp"
#include "memory/fifo_replacement.hpp"
#include "memory/lru_replacement.hpp"
#include "memory/optimal_replacement.hpp"
#include "memory/nru_replacement.hpp"
#include "core/process.hpp"

using namespace OSSimulator;

TEST_CASE("MemoryManager Initialization", "[memory]") {
    auto algo = std::make_unique<FIFOReplacement>();
    MemoryManager mm(10, std::move(algo));
    
    REQUIRE(mm.get_total_page_faults() == 0);
    REQUIRE(mm.get_total_replacements() == 0);
}

TEST_CASE("Memory Allocation and Page Faults", "[memory]") {
    auto algo = std::make_unique<FIFOReplacement>();
    MemoryManager mm(2, std::move(algo)); // Only 2 frames
    
    auto proc = std::make_shared<Process>(1, "P1", 0, 10, 0, 5); // Needs 5 pages
    mm.allocate_initial_memory(*proc);
    mm.register_process(proc);
    
    // Access page 0 -> Fault
    REQUIRE_FALSE(mm.request_page(*proc, 0, false, 0));
    mm.handle_page_fault(*proc, 0, 0);
    REQUIRE(mm.request_page(*proc, 0, false, 0)); // Should be hit now
    REQUIRE(mm.get_total_page_faults() == 1);
    
    // Access page 1 -> Fault
    REQUIRE_FALSE(mm.request_page(*proc, 1, false, 1));
    mm.handle_page_fault(*proc, 1, 1);
    REQUIRE(mm.request_page(*proc, 1, false, 1));
    REQUIRE(mm.get_total_page_faults() == 2);
    
    // Access page 2 -> Fault + Replacement (FIFO should evict page 0)
    REQUIRE_FALSE(mm.request_page(*proc, 2, false, 2));
    mm.handle_page_fault(*proc, 2, 2);
    REQUIRE(mm.get_total_page_faults() == 3);
    REQUIRE(mm.get_total_replacements() == 1);
    
    // Page 0 should be gone
    REQUIRE_FALSE(mm.request_page(*proc, 0, false, 3));
}

TEST_CASE("LRU Replacement", "[memory]") {
    auto algo = std::make_unique<LRUReplacement>();
    MemoryManager mm(2, std::move(algo));
    
    auto proc = std::make_shared<Process>(1, "P1", 0, 10, 0, 5);
    mm.allocate_initial_memory(*proc);
    mm.register_process(proc);
    
    // Load 0 and 1
    mm.handle_page_fault(*proc, 0, 0);
    mm.handle_page_fault(*proc, 1, 1);
    
    // Access 0 again (update LRU)
    mm.request_page(*proc, 0, false, 2);
    
    // Load 2 -> Should evict 1 (since 0 was just used)
    mm.handle_page_fault(*proc, 2, 3);
    
    REQUIRE(mm.request_page(*proc, 0, false, 4)); // 0 should still be there
    REQUIRE_FALSE(mm.request_page(*proc, 1, false, 4)); // 1 should be gone
}

TEST_CASE("Optimal Replacement", "[memory]") {
    auto algo = std::make_unique<OptimalReplacement>();
    MemoryManager mm(2, std::move(algo));
    
    auto proc = std::make_shared<Process>(1, "P1", 0, 10, 0, 5);
    // Trace: 0, 1, 0, 2, 0, 1
    proc->memory_access_trace = {0, 1, 0, 2, 0, 1};
    mm.allocate_initial_memory(*proc);
    mm.register_process(proc);
    
    // Load 0 (index 0)
    mm.handle_page_fault(*proc, 0, 0);
    proc->advance_page_access();
    
    // Load 1 (index 1)
    mm.handle_page_fault(*proc, 1, 1);
    proc->advance_page_access();
    
    // Access 0 (index 2) -> Hit
    mm.request_page(*proc, 0, false, 2);
    proc->advance_page_access();
    
    // Load 2 (index 3) -> Replacement
    // Future: 0 (soon), 1 (later). Should evict 1.
    mm.handle_page_fault(*proc, 2, 3);
    proc->advance_page_access();
    
    REQUIRE(mm.request_page(*proc, 0, false, 4)); // 0 should be there
    REQUIRE_FALSE(mm.request_page(*proc, 1, false, 4)); // 1 evicted
}
