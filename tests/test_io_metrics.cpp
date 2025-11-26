#include <catch2/catch_test_macros.hpp>
#include "metrics/metrics_collector.hpp"
#include "core/process.hpp"
#include "io/io_manager.hpp"
#include "io/io_device.hpp"
#include "io/io_fcfs_scheduler.hpp"
#include "io/io_request.hpp"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace OSSimulator;
using json = nlohmann::json;

TEST_CASE("MetricsCollector initialization", "[metrics][init]") {
  SECTION("Default state is disabled") {
    MetricsCollector metrics;
    REQUIRE_FALSE(metrics.is_enabled());
  }

  SECTION("Enable file output successfully") {
    std::filesystem::create_directories("data/resultados");
    const std::string path = "data/resultados/test_metrics.jsonl";
    
    if (std::filesystem::exists(path)) {
      std::filesystem::remove(path);
    }

    MetricsCollector metrics;
    REQUIRE(metrics.enable_file_output(path));
    REQUIRE(metrics.is_enabled());
    
    metrics.disable_output();
    REQUIRE_FALSE(metrics.is_enabled());
  }

  SECTION("Enable stdout output") {
    MetricsCollector metrics;
    metrics.enable_stdout_output();
    REQUIRE(metrics.is_enabled());
  }

  SECTION("Invalid path fails gracefully") {
    MetricsCollector metrics;
    REQUIRE_FALSE(metrics.enable_file_output("/invalid/path/to/file.jsonl"));
    REQUIRE_FALSE(metrics.is_enabled());
  }
}

TEST_CASE("MetricsCollector CPU logging", "[metrics][cpu]") {
  std::filesystem::create_directories("data/resultados");
  const std::string path = "data/resultados/test_cpu_metrics.jsonl";
  
  if (std::filesystem::exists(path)) {
    std::filesystem::remove(path);
  }

  MetricsCollector metrics;
  REQUIRE(metrics.enable_file_output(path));

  SECTION("Log single CPU event") {
    metrics.log_cpu(0, "EXEC", 1, "P1", 10, 2, false);
    metrics.flush_all();
    metrics.disable_output();

    std::ifstream in(path);
    REQUIRE(in.is_open());
    
    std::string line;
    REQUIRE(std::getline(in, line));
    REQUIRE_FALSE(line.empty());

    json j = json::parse(line);
    REQUIRE(j["tick"] == 0);
    REQUIRE(j.contains("cpu"));
    REQUIRE(j["cpu"]["event"] == "EXEC");
    REQUIRE(j["cpu"]["pid"] == 1);
    REQUIRE(j["cpu"]["name"] == "P1");
    REQUIRE(j["cpu"]["remaining"] == 10);
    REQUIRE(j["cpu"]["ready_queue"] == 2);
    REQUIRE(j["cpu"]["context_switch"] == false);
  }

  SECTION("Log CPU with context switch") {
    metrics.log_cpu(5, "PREEMPT", 2, "P2", 8, 3, true);
    metrics.flush_all();
    metrics.disable_output();

    std::ifstream in(path);
    std::string line;
    REQUIRE(std::getline(in, line));

    json j = json::parse(line);
    REQUIRE(j["tick"] == 5);
    REQUIRE(j["cpu"]["event"] == "PREEMPT");
    REQUIRE(j["cpu"]["pid"] == 2);
    REQUIRE(j["cpu"]["name"] == "P2");
    REQUIRE(j["cpu"]["remaining"] == 8);
    REQUIRE(j["cpu"]["ready_queue"] == 3);
    REQUIRE(j["cpu"]["context_switch"] == true);
  }

  SECTION("Log multiple CPU events in sequence") {
    metrics.log_cpu(0, "EXEC", 1, "P1", 10, 1, false);
    metrics.log_cpu(1, "EXEC", 1, "P1", 9, 1, false);
    metrics.log_cpu(2, "COMPLETE", 1, "P1", 0, 0, false);
    metrics.flush_all();
    metrics.disable_output();

    std::ifstream in(path);
    std::string line;
    int line_count = 0;
    
    while (std::getline(in, line)) {
      if (!line.empty()) {
        json j = json::parse(line);
        REQUIRE(j.contains("cpu"));
        REQUIRE(j["cpu"]["pid"] == 1);
        REQUIRE(j["cpu"]["name"] == "P1");
        line_count++;
      }
    }
    
    REQUIRE(line_count == 3);
  }
}

TEST_CASE("MetricsCollector IO logging", "[metrics][io]") {
  std::filesystem::create_directories("data/resultados");
  const std::string path = "data/resultados/test_io_metrics.jsonl";
  
  if (std::filesystem::exists(path)) {
    std::filesystem::remove(path);
  }

  MetricsCollector metrics;
  REQUIRE(metrics.enable_file_output(path));

  SECTION("Log single IO event") {
    metrics.log_io(0, "disk", "IO_START", 1, "P1", 5, 2);
    metrics.flush_all();
    metrics.disable_output();

    std::ifstream in(path);
    REQUIRE(in.is_open());
    
    std::string line;
    REQUIRE(std::getline(in, line));
    REQUIRE_FALSE(line.empty());

    json j = json::parse(line);
    REQUIRE(j["tick"] == 0);
    REQUIRE(j.contains("io"));
    REQUIRE(j["io"]["device"] == "disk");
    REQUIRE(j["io"]["event"] == "IO_START");
    REQUIRE(j["io"]["pid"] == 1);
    REQUIRE(j["io"]["name"] == "P1");
    REQUIRE(j["io"]["remaining"] == 5);
    REQUIRE(j["io"]["queue"] == 2);
  }

  SECTION("Log IO completion event") {
    metrics.log_io(10, "tape", "IO_COMPLETE", 3, "P3", 0, 0);
    metrics.flush_all();
    metrics.disable_output();

    std::ifstream in(path);
    std::string line;
    REQUIRE(std::getline(in, line));

    json j = json::parse(line);
    REQUIRE(j["tick"] == 10);
    REQUIRE(j["io"]["device"] == "tape");
    REQUIRE(j["io"]["event"] == "IO_COMPLETE");
    REQUIRE(j["io"]["pid"] == 3);
    REQUIRE(j["io"]["name"] == "P3");
    REQUIRE(j["io"]["remaining"] == 0);
    REQUIRE(j["io"]["queue"] == 0);
  }

  SECTION("Log multiple IO events on different devices") {
    metrics.log_io(0, "disk", "IO_START", 1, "P1", 5, 1);
    metrics.log_io(1, "tape", "IO_START", 2, "P2", 3, 0);
    metrics.log_io(2, "disk", "IO_EXEC", 1, "P1", 4, 1);
    metrics.flush_all();
    metrics.disable_output();

    std::ifstream in(path);
    std::string line;
    int line_count = 0;
    
    while (std::getline(in, line)) {
      if (!line.empty()) {
        json j = json::parse(line);
        REQUIRE(j.contains("io"));
        line_count++;
      }
    }
    
    REQUIRE(line_count == 3);
  }
}

TEST_CASE("MetricsCollector combined CPU and IO logging", "[metrics][combined]") {
  std::filesystem::create_directories("data/resultados");
  const std::string path = "data/resultados/test_combined_metrics.jsonl";
  
  if (std::filesystem::exists(path)) {
    std::filesystem::remove(path);
  }

  MetricsCollector metrics;
  REQUIRE(metrics.enable_file_output(path));

  SECTION("CPU and IO events in same tick") {
    metrics.log_cpu(0, "EXEC", 1, "P1", 10, 2, false);
    metrics.log_io(0, "disk", "IO_START", 2, "P2", 5, 1);
    metrics.flush_all();
    metrics.disable_output();

    std::ifstream in(path);
    std::string line;
    REQUIRE(std::getline(in, line));

    json j = json::parse(line);
    REQUIRE(j["tick"] == 0);
    REQUIRE(j.contains("cpu"));
    REQUIRE(j.contains("io"));
    
    REQUIRE(j["cpu"]["event"] == "EXEC");
    REQUIRE(j["cpu"]["pid"] == 1);
    REQUIRE(j["cpu"]["name"] == "P1");
    
    REQUIRE(j["io"]["device"] == "disk");
    REQUIRE(j["io"]["event"] == "IO_START");
    REQUIRE(j["io"]["pid"] == 2);
    REQUIRE(j["io"]["name"] == "P2");
  }

  SECTION("Multiple ticks with mixed events") {
    // Tick 0: CPU only
    metrics.log_cpu(0, "EXEC", 1, "P1", 10, 1, false);
    
    // Tick 1: IO only
    metrics.log_io(1, "disk", "IO_START", 2, "P2", 5, 0);
    
    // Tick 2: Both CPU and IO
    metrics.log_cpu(2, "EXEC", 1, "P1", 9, 1, false);
    metrics.log_io(2, "disk", "IO_EXEC", 2, "P2", 4, 0);
    
    metrics.flush_all();
    metrics.disable_output();

    std::ifstream in(path);
    std::string line;
    
    // Line 1: tick 0, CPU only
    REQUIRE(std::getline(in, line));
    json j0 = json::parse(line);
    REQUIRE(j0["tick"] == 0);
    REQUIRE(j0.contains("cpu"));
    REQUIRE_FALSE(j0.contains("io"));
    
    // Line 2: tick 1, IO only
    REQUIRE(std::getline(in, line));
    json j1 = json::parse(line);
    REQUIRE(j1["tick"] == 1);
    REQUIRE_FALSE(j1.contains("cpu"));
    REQUIRE(j1.contains("io"));
    
    // Line 3: tick 2, both
    REQUIRE(std::getline(in, line));
    json j2 = json::parse(line);
    REQUIRE(j2["tick"] == 2);
    REQUIRE(j2.contains("cpu"));
    REQUIRE(j2.contains("io"));
  }
}

TEST_CASE("MetricsCollector tick ordering and buffering", "[metrics][buffer]") {
  std::filesystem::create_directories("data/resultados");
  const std::string path = "data/resultados/test_buffer_metrics.jsonl";
  
  if (std::filesystem::exists(path)) {
    std::filesystem::remove(path);
  }

  MetricsCollector metrics;
  REQUIRE(metrics.enable_file_output(path));

  SECTION("Ticks are flushed in order") {
    // Log events out of order
    metrics.log_cpu(2, "EXEC", 3, "P3", 5, 0, false);
    metrics.log_cpu(0, "EXEC", 1, "P1", 10, 2, false);
    metrics.log_cpu(1, "EXEC", 2, "P2", 8, 1, false);
    
    metrics.flush_all();
    metrics.disable_output();

    std::ifstream in(path);
    std::string line;
    
    // Should be flushed in tick order: 0, 1, 2
    REQUIRE(std::getline(in, line));
    json j0 = json::parse(line);
    REQUIRE(j0["tick"] == 0);
    REQUIRE(j0["cpu"]["pid"] == 1);
    
    REQUIRE(std::getline(in, line));
    json j1 = json::parse(line);
    REQUIRE(j1["tick"] == 1);
    REQUIRE(j1["cpu"]["pid"] == 2);
    
    REQUIRE(std::getline(in, line));
    json j2 = json::parse(line);
    REQUIRE(j2["tick"] == 2);
    REQUIRE(j2["cpu"]["pid"] == 3);
  }

  SECTION("Multiple logs to same tick are merged") {
    metrics.log_cpu(5, "EXEC", 1, "P1", 10, 1, false);
    metrics.log_io(5, "disk", "IO_START", 2, "P2", 5, 0);
    
    metrics.flush_all();
    metrics.disable_output();

    std::ifstream in(path);
    std::string line;
    int line_count = 0;
    
    while (std::getline(in, line)) {
      if (!line.empty()) {
        json j = json::parse(line);
        REQUIRE(j["tick"] == 5);
        REQUIRE(j.contains("cpu"));
        REQUIRE(j.contains("io"));
        line_count++;
      }
    }
    
    // Should be only one line with both CPU and IO
    REQUIRE(line_count == 1);
  }
}

TEST_CASE("MetricsCollector with IO Manager integration", "[metrics][integration]") {
  std::filesystem::create_directories("data/resultados");
  const std::string path = "data/resultados/test_integration_metrics.jsonl";

  if (std::filesystem::exists(path)) {
    std::filesystem::remove(path);
  }

  auto metrics = std::make_shared<MetricsCollector>();
  REQUIRE(metrics->enable_file_output(path));

  IOManager manager;
  manager.set_metrics_collector(metrics);

  auto dev = std::make_shared<IODevice>("disk");
  dev->set_scheduler(std::make_unique<IOFCFSScheduler>());
  manager.add_device("disk", dev);

  auto p1 = std::make_shared<Process>(1, "P1", 0, 10);
  Burst b1(BurstType::IO, 3, "disk");
  auto r1 = std::make_shared<IORequest>(p1, b1, 0);

  auto p2 = std::make_shared<Process>(2, "P2", 1, 5);
  Burst b2(BurstType::IO, 2, "disk");
  auto r2 = std::make_shared<IORequest>(p2, b2, 1);

  manager.submit_io_request(r1);
  manager.submit_io_request(r2);

  int current_time = 0;
  int quantum = 1;

  while (manager.has_pending_io()) {
    manager.execute_all_devices(quantum, current_time);
    current_time += quantum;
  }

  metrics->flush_all();
  metrics->disable_output();

  SECTION("File is created and contains valid JSON") {
    REQUIRE(std::filesystem::exists(path));

    std::ifstream in(path);
    REQUIRE(in.is_open());

    std::string line;
    int valid_lines = 0;
    
    while (std::getline(in, line)) {
      if (!line.empty()) {
        // Verify each line is valid JSON
        json j;
        REQUIRE_NOTHROW(j = json::parse(line));
        
        REQUIRE(j.contains("tick"));
        REQUIRE(j["tick"].is_number());
        
        valid_lines++;
      }
    }

    REQUIRE(valid_lines >= 1);
  }

  SECTION("Events contain correct structure") {
    std::ifstream in(path);
    std::string line;
    
    while (std::getline(in, line)) {
      if (!line.empty()) {
        json j = json::parse(line);
        
        // If IO event exists, check structure
        if (j.contains("io")) {
          REQUIRE(j["io"].contains("device"));
          REQUIRE(j["io"].contains("event"));
          REQUIRE(j["io"].contains("pid"));
          REQUIRE(j["io"].contains("name"));
          REQUIRE(j["io"].contains("remaining"));
          REQUIRE(j["io"].contains("queue"));
          
          REQUIRE(j["io"]["device"].is_string());
          REQUIRE(j["io"]["event"].is_string());
          REQUIRE(j["io"]["pid"].is_number());
          REQUIRE(j["io"]["name"].is_string());
          REQUIRE(j["io"]["remaining"].is_number());
          REQUIRE(j["io"]["queue"].is_number());
        }
      }
    }
  }
}

TEST_CASE("MetricsCollector output modes", "[metrics][modes]") {
  SECTION("Switching from file to stdout") {
    std::filesystem::create_directories("data/resultados");
    const std::string path = "data/resultados/test_mode_switch.jsonl";
    
    if (std::filesystem::exists(path)) {
      std::filesystem::remove(path);
    }

    MetricsCollector metrics;
    REQUIRE(metrics.enable_file_output(path));
    REQUIRE(metrics.is_enabled());
    
    // Switch to stdout
    metrics.enable_stdout_output();
    REQUIRE(metrics.is_enabled());
    
    // Log something
    // TODO: great job at polluting test output :+1:
    metrics.log_cpu(0, "EXEC", 1, "P1", 10, 0, false);
    metrics.flush_all();
    
    metrics.disable_output();
    REQUIRE_FALSE(metrics.is_enabled());
  }

  SECTION("Disable output clears buffer") {
    std::filesystem::create_directories("data/resultados");
    const std::string path = "data/resultados/test_disable.jsonl";
    
    if (std::filesystem::exists(path)) {
      std::filesystem::remove(path);
    }

    MetricsCollector metrics;
    REQUIRE(metrics.enable_file_output(path));
    
    metrics.log_cpu(0, "EXEC", 1, "P1", 10, 0, false);
    metrics.disable_output();
    
    REQUIRE_FALSE(metrics.is_enabled());
    REQUIRE(std::filesystem::exists(path));
  }
}
