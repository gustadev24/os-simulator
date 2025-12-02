#include "core/process.hpp"
#include "metrics/metrics_collector.hpp"
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace OSSimulator;
using json = nlohmann::json;

TEST_CASE("State Transition Metrics - Basic Logging",
          "[metrics][state_transition]") {
  std::filesystem::create_directories("data/test/resultados");
  const std::string path =
      "data/test/resultados/test_state_transition_metrics.jsonl";

  if (std::filesystem::exists(path)) {
    std::filesystem::remove(path);
  }

  auto metrics = std::make_shared<MetricsCollector>();
  REQUIRE(metrics->enable_file_output(path));

  SECTION("Log NEW to READY transition") {
    metrics->log_state_transition(0, 1, "P1", ProcessState::NEW,
                                  ProcessState::READY, "arrival");
    metrics->flush_all();
    metrics->disable_output();

    std::ifstream in(path);
    REQUIRE(in.is_open());

    std::string line;
    REQUIRE(std::getline(in, line));

    json j = json::parse(line);
    REQUIRE(j["tick"] == 0);
    REQUIRE(j["state_transitions"].is_array());
    REQUIRE(j["state_transitions"].size() >= 1);
    auto &st = j["state_transitions"][0];
    REQUIRE(st["pid"] == 1);
    REQUIRE(st["name"] == "P1");
    REQUIRE(st["from"] == "NEW");
    REQUIRE(st["to"] == "READY");
    REQUIRE(st["reason"] == "arrival");
  }

  SECTION("Log READY to RUNNING transition") {
    metrics->log_state_transition(5, 2, "P2", ProcessState::READY,
                                  ProcessState::RUNNING, "scheduled");
    metrics->flush_all();
    metrics->disable_output();

    std::ifstream in(path);
    std::string line;
    REQUIRE(std::getline(in, line));

    json j = json::parse(line);
    REQUIRE(j["state_transitions"].is_array());
    REQUIRE(j["state_transitions"].size() >= 1);
    auto &st = j["state_transitions"][0];
    REQUIRE(st["from"] == "READY");
    REQUIRE(st["to"] == "RUNNING");
    REQUIRE(st["reason"] == "scheduled");
  }

  SECTION("Log RUNNING to MEMORY_WAITING transition") {
    metrics->log_state_transition(10, 3, "P3", ProcessState::RUNNING,
                                  ProcessState::MEMORY_WAITING, "page_fault");
    metrics->flush_all();
    metrics->disable_output();

    std::ifstream in(path);
    std::string line;
    REQUIRE(std::getline(in, line));

    json j = json::parse(line);
    REQUIRE(j["state_transitions"][0]["from"] == "RUNNING");
    REQUIRE(j["state_transitions"][0]["to"] == "MEMORY_WAITING");
    REQUIRE(j["state_transitions"][0]["reason"] == "page_fault");
  }

  SECTION("Log MEMORY_WAITING to READY transition") {
    metrics->log_state_transition(15, 3, "P3", ProcessState::MEMORY_WAITING,
                                  ProcessState::READY, "memory_loaded");
    metrics->flush_all();
    metrics->disable_output();

    std::ifstream in(path);
    std::string line;
    REQUIRE(std::getline(in, line));

    json j = json::parse(line);
    REQUIRE(j["state_transitions"][0]["from"] == "MEMORY_WAITING");
    REQUIRE(j["state_transitions"][0]["to"] == "READY");
    REQUIRE(j["state_transitions"][0]["reason"] == "memory_loaded");
  }

  SECTION("Log RUNNING to WAITING transition (I/O)") {
    metrics->log_state_transition(20, 4, "P4", ProcessState::RUNNING,
                                  ProcessState::WAITING, "io_request");
    metrics->flush_all();
    metrics->disable_output();

    std::ifstream in(path);
    std::string line;
    REQUIRE(std::getline(in, line));

    json j = json::parse(line);
    REQUIRE(j["state_transitions"][0]["from"] == "RUNNING");
    REQUIRE(j["state_transitions"][0]["to"] == "WAITING");
    REQUIRE(j["state_transitions"][0]["reason"] == "io_request");
  }

  SECTION("Log WAITING to READY transition") {
    metrics->log_state_transition(25, 4, "P4", ProcessState::WAITING,
                                  ProcessState::READY, "io_completed");
    metrics->flush_all();
    metrics->disable_output();

    std::ifstream in(path);
    std::string line;
    REQUIRE(std::getline(in, line));

    json j = json::parse(line);
    REQUIRE(j["state_transitions"][0]["from"] == "WAITING");
    REQUIRE(j["state_transitions"][0]["to"] == "READY");
    REQUIRE(j["state_transitions"][0]["reason"] == "io_completed");
  }

  SECTION("Log RUNNING to TERMINATED transition") {
    metrics->log_state_transition(30, 5, "P5", ProcessState::RUNNING,
                                  ProcessState::TERMINATED, "completed");
    metrics->flush_all();
    metrics->disable_output();

    std::ifstream in(path);
    std::string line;
    REQUIRE(std::getline(in, line));

    json j = json::parse(line);
    REQUIRE(j["state_transitions"][0]["from"] == "RUNNING");
    REQUIRE(j["state_transitions"][0]["to"] == "TERMINATED");
    REQUIRE(j["state_transitions"][0]["reason"] == "completed");
  }
}

TEST_CASE("State Transition Metrics - Multiple Transitions",
          "[metrics][state_transition]") {
  std::filesystem::create_directories("data/test/resultados");
  const std::string path =
      "data/test/resultados/test_multiple_state_transitions.jsonl";

  if (std::filesystem::exists(path)) {
    std::filesystem::remove(path);
  }

  auto metrics = std::make_shared<MetricsCollector>();
  REQUIRE(metrics->enable_file_output(path));

  SECTION("Log process lifecycle") {
    metrics->log_state_transition(0, 1, "P1", ProcessState::NEW,
                                  ProcessState::READY, "arrival");
    metrics->log_state_transition(5, 1, "P1", ProcessState::READY,
                                  ProcessState::RUNNING, "scheduled");
    metrics->log_state_transition(10, 1, "P1", ProcessState::RUNNING,
                                  ProcessState::WAITING, "io_request");
    metrics->log_state_transition(15, 1, "P1", ProcessState::WAITING,
                                  ProcessState::READY, "io_completed");
    metrics->log_state_transition(20, 1, "P1", ProcessState::READY,
                                  ProcessState::RUNNING, "scheduled");
    metrics->log_state_transition(25, 1, "P1", ProcessState::RUNNING,
                                  ProcessState::TERMINATED, "completed");
    metrics->flush_all();
    metrics->disable_output();

    std::ifstream in(path);
    REQUIRE(in.is_open());

    int transition_count = 0;
    std::string line;
    while (std::getline(in, line)) {
      json j = json::parse(line);
      if (j.contains("state_transitions")) {
        transition_count++;
        REQUIRE(j["state_transitions"][0]["pid"] == 1);
        REQUIRE(j["state_transitions"][0]["name"] == "P1");
      }
    }

    REQUIRE(transition_count == 6);
  }
}

TEST_CASE("Combined Metrics - CPU, Memory, and State Transitions",
          "[metrics][integration]") {
  std::filesystem::create_directories("data/test/resultados");
  const std::string path = "data/test/resultados/test_combined_metrics.jsonl";

  if (std::filesystem::exists(path)) {
    std::filesystem::remove(path);
  }

  auto metrics = std::make_shared<MetricsCollector>();
  REQUIRE(metrics->enable_file_output(path));

  SECTION("Log combined events at same tick") {
    metrics->log_state_transition(10, 1, "P1", ProcessState::READY,
                                  ProcessState::RUNNING, "scheduled");
    metrics->log_cpu(10, "EXEC", 1, "P1", 5, 2, true);

    metrics->flush_all();
    metrics->disable_output();

    std::ifstream in(path);
    REQUIRE(in.is_open());

    std::string line;
    REQUIRE(std::getline(in, line));

    json j = json::parse(line);
    REQUIRE(j["tick"] == 10);
    REQUIRE(j.contains("cpu"));
    REQUIRE(j.contains("state_transitions"));
    REQUIRE(j["cpu"]["event"] == "EXEC");
    REQUIRE(j["state_transitions"][0]["to"] == "RUNNING");
  }

  SECTION("Log memory and state transition together") {
    metrics->log_memory(15, "PAGE_FAULT", 2, "P2", 3, -1, 5, 0);
    metrics->log_state_transition(15, 2, "P2", ProcessState::RUNNING,
                                  ProcessState::MEMORY_WAITING, "page_fault");

    metrics->flush_all();
    metrics->disable_output();

    std::ifstream in(path);
    std::string line;
    REQUIRE(std::getline(in, line));

    json j = json::parse(line);
    REQUIRE(j["tick"] == 15);
    REQUIRE(j.contains("memory"));
    REQUIRE(j.contains("state_transitions"));
    REQUIRE(j["memory"]["event"] == "PAGE_FAULT");
    REQUIRE(j["state_transitions"][0]["to"] == "MEMORY_WAITING");
  }
}
