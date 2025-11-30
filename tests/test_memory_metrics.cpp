#include "core/process.hpp"
#include "memory/fifo_replacement.hpp"
#include "memory/memory_manager.hpp"
#include "metrics/metrics_collector.hpp"
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace OSSimulator;
using json = nlohmann::json;

TEST_CASE("Memory Metrics - Page Fault Logging", "[metrics][memory]") {
  std::filesystem::create_directories("data/test/resultados");
  const std::string path = "data/test/resultados/test_memory_metrics.jsonl";

  if (std::filesystem::exists(path)) {
    std::filesystem::remove(path);
  }

  auto metrics = std::make_shared<MetricsCollector>();
  REQUIRE(metrics->enable_file_output(path));

  SECTION("Log page fault event") {
    metrics->log_memory(10, "PAGE_FAULT", 1, "P1", 3, -1, 5, 0);
    metrics->flush_all();
    metrics->disable_output();

    std::ifstream in(path);
    REQUIRE(in.is_open());

    std::string line;
    REQUIRE(std::getline(in, line));

    json j = json::parse(line);
    REQUIRE(j["tick"] == 10);
    REQUIRE(j["memory"]["event"] == "PAGE_FAULT");
    REQUIRE(j["memory"]["pid"] == 1);
    REQUIRE(j["memory"]["name"] == "P1");
    REQUIRE(j["memory"]["page_id"] == 3);
    REQUIRE(j["memory"]["frame_id"] == -1);
    REQUIRE(j["memory"]["total_page_faults"] == 5);
    REQUIRE(j["memory"]["total_replacements"] == 0);
  }

  SECTION("Log page loaded event") {
    metrics->log_memory(15, "PAGE_LOADED", 2, "P2", 5, 10, 8, 2);
    metrics->flush_all();
    metrics->disable_output();

    std::ifstream in(path);
    std::string line;
    REQUIRE(std::getline(in, line));

    json j = json::parse(line);
    REQUIRE(j["memory"]["event"] == "PAGE_LOADED");
    REQUIRE(j["memory"]["frame_id"] == 10);
    REQUIRE(j["memory"]["total_page_faults"] == 8);
  }

  SECTION("Log page replacement") {
    metrics->log_memory(20, "PAGE_REPLACED", 3, "P3", 7, 15, 12, 5);
    metrics->flush_all();
    metrics->disable_output();

    std::ifstream in(path);
    std::string line;
    REQUIRE(std::getline(in, line));

    json j = json::parse(line);
    REQUIRE(j["memory"]["event"] == "PAGE_REPLACED");
    REQUIRE(j["memory"]["page_id"] == 7);
    REQUIRE(j["memory"]["frame_id"] == 15);
    REQUIRE(j["memory"]["total_replacements"] == 5);
  }
}

TEST_CASE("Memory Metrics - Summary", "[metrics][memory]") {
  std::filesystem::create_directories("data/test/resultados");
  const std::string path = "data/test/resultados/test_memory_summary.jsonl";

  if (std::filesystem::exists(path)) {
    std::filesystem::remove(path);
  }

  auto metrics = std::make_shared<MetricsCollector>();
  REQUIRE(metrics->enable_file_output(path));

  SECTION("Log memory summary") {
    metrics->log_memory_summary(25, 10, 64, 48, "LRU");
    metrics->flush_all();
    metrics->disable_output();

    std::ifstream in(path);
    REQUIRE(in.is_open());

    std::string line;
    REQUIRE(std::getline(in, line));

    json j = json::parse(line);
    REQUIRE(j["summary"] == "MEMORY_METRICS");
    REQUIRE(j["total_page_faults"] == 25);
    REQUIRE(j["total_replacements"] == 10);
    REQUIRE(j["total_frames"] == 64);
    REQUIRE(j["used_frames"] == 48);
    REQUIRE(j["frame_utilization"] == 75.0);
    REQUIRE(j["algorithm"] == "LRU");
  }
}

TEST_CASE("Memory Metrics - Integration with MemoryManager",
          "[metrics][memory][integration]") {
  std::filesystem::create_directories("data/test/resultados");
  const std::string path = "data/test/resultados/test_memory_integration.jsonl";

  if (std::filesystem::exists(path)) {
    std::filesystem::remove(path);
  }

  auto metrics = std::make_shared<MetricsCollector>();
  REQUIRE(metrics->enable_file_output(path));

  auto mm = std::make_shared<MemoryManager>(4, std::make_unique<FIFOReplacement>(), 1);
  mm->set_metrics_collector(metrics);

  SECTION("Memory manager logs page faults") {
    auto p1 = std::make_shared<Process>(1, "P1", 0, 10, 0, 3);
    mm->register_process(p1);
    mm->allocate_initial_memory(*p1);

    mm->prepare_process_for_cpu(p1, 0);

    metrics->flush_all();

    std::ifstream in(path);
    REQUIRE(in.is_open());

    bool found_page_fault = false;
    std::string line;
    while (std::getline(in, line)) {
      json j = json::parse(line);
      if (j.contains("memory") && j["memory"]["event"] == "PAGE_FAULT") {
        found_page_fault = true;
        REQUIRE(j["memory"]["pid"] == 1);
        REQUIRE(j["memory"]["name"] == "P1");
        break;
      }
    }

    REQUIRE(found_page_fault);
    mm->set_metrics_collector(nullptr);
    metrics->disable_output();
  }
}
