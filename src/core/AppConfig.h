#pragma once

#include <string>
#include <vector>

namespace ulak::core {

// Determines which transport backends are instantiated at startup.
// Controlled by "deployment_mode" in settings.json.
enum class DeploymentMode {
  kSimulation,  // All data channels use ROS2 via rosbridge WebSocket.
  kReal,        // All data channels use direct serial/TCP connections.
};

// ── Simulation-mode transport ────────────────────────────────────────────────

// rosbridge WebSocket endpoint (simulation mode only).
struct Ros2BridgeConfig {
  std::string host;
  int port{0};
};

// ROS2 topic name mappings (simulation mode only).
// All names are user-configurable via settings.json and the UI topic panel.
struct Ros2TopicsConfig {
  std::string telemetry;
  std::string vehicle_state;
  std::string mission_state;
  std::string perception_output;
  std::string safety_events;
  std::string camera;
  std::string commands;
};

struct Ros2Config {
  Ros2BridgeConfig bridge;
  Ros2TopicsConfig topics;
};

// ── Real-mode transport ──────────────────────────────────────────────────────

// MAVLink serial connection to Pixhawk (real mode only).
struct SerialEndpointConfig {
  std::string port;  // e.g. "COM3" on Windows, "/dev/ttyUSB0" on Linux
  int baud{0};
};

// TCP connection endpoint (companion computer, command channel).
struct TcpEndpointConfig {
  std::string host;
  int port{0};
};

// H.264/H.265 camera stream config (real mode only).
struct StreamConfig {
  std::string mode;  // OFF | OUTPUTS_ONLY | COMPRESSED_LIVE | RAW_DEBUG
  std::string host;
  int port{0};
  std::string codec;
  int fps{0};
  int bitrate_kbps{0};
};

// Network config block (real mode only).
struct NetworkConfig {
  SerialEndpointConfig telemetry_endpoint;  // MAVLink serial to Pixhawk FC
  TcpEndpointConfig companion_endpoint;     // JSON-over-TCP from Raspberry Pi CC
  TcpEndpointConfig command_endpoint;       // JSON-over-TCP to Raspberry Pi CC
  StreamConfig stream;
};

// ── Simulation coordinate transform ─────────────────────────────────────────

// 3-axis floating-point vector.
struct Vec3Config {
  double x{0.0};
  double y{0.0};
  double z{0.0};
};

// Axis remapping for Gazebo-world → ArduPilot-LOCAL_NED transform.
struct AxisMapConfig {
  std::string x;  // e.g. "y"
  std::string y;  // e.g. "x"
  std::string z;  // e.g. "-z"
};

// Coordinate transform applied to simulation telemetry before TelemetryFrame is populated.
struct CoordinateTransformConfig {
  bool enabled{false};
  std::string source_frame_id;
  std::string target_frame_id;
  AxisMapConfig axis_map;
  Vec3Config translation_m;
  Vec3Config rotation_deg;
  Vec3Config scale;
};

// Simulation-specific config block (simulation mode only).
struct SimulationConfig {
  std::string active_transform_id;
  CoordinateTransformConfig coordinate_transform;
};

// ── Shared config ────────────────────────────────────────────────────────────

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

// ── Root config ──────────────────────────────────────────────────────────────

struct AppConfig {
  std::string schema_version;
  std::string instance_name;
  std::string active_profile;
  DeploymentMode deployment_mode{DeploymentMode::kSimulation};

  Ros2Config ros2;        // Always loaded; active when deployment_mode == kSimulation.
  NetworkConfig network;  // Always loaded; active when deployment_mode == kReal.
  SimulationConfig simulation;
  CommandLifecycleConfig command_lifecycle;
  SafetyConfig safety;
  LoggingConfig logging;
};

}  // namespace ulak::core
