#pragma once

#include <string>
#include <vector>

namespace ulak::core {

struct EndpointConfig {
  std::string transport;
  std::string host;
  int port{0};
};

struct TelemetryConfig {
  EndpointConfig vehicle_endpoint;
  EndpointConfig simulator_endpoint;
  int health_interval_ms{0};
};

struct CompanionConfig {
  EndpointConfig endpoint;
  EndpointConfig command_endpoint;
};

struct StreamConfig {
  std::string mode;
  int fps{0};
  int bitrate_kbps{0};
  bool raw_debug_enabled{false};
};

struct CommandRetryConfig {
  int max_attempts{0};
  std::vector<int> backoff_ms;
};

struct CommandLifecycleConfig {
  int ack_timeout_ms{0};
  int exec_timeout_ms{0};
  CommandRetryConfig retry;
};

struct SafetyConfig {
  int error_confirm_window_sec{0};
  bool panic_requires_guard{false};
  int panic_lockout_ms{0};
};

struct LoggingConfig {
  std::string directory;
  bool ndjson_enabled{false};
  std::string session_rotation;
};

struct AppConfig {
  std::string schema_version;
  std::string instance_name;
  std::string active_profile;

  TelemetryConfig telemetry;
  CompanionConfig companion;
  StreamConfig stream;
  CommandLifecycleConfig command_lifecycle;
  SafetyConfig safety;
  LoggingConfig logging;
};

}  // namespace ulak::core
