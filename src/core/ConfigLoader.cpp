#include "ConfigLoader.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_set>

#include "AppConfig.h"
#include "JsonUtils.h"

namespace ulak::core {
namespace {

ConfigResult MakeError(ConfigError error, std::string message) {
  ConfigResult result;
  result.ok = false;
  result.error = error;
  result.message = std::move(message);
  return result;
}

// ── Field helpers ────────────────────────────────────────────────────────────

bool RequireStringField(const ulak::json::Value& object,
                        const std::string& key,
                        std::string* out,
                        ConfigResult* error) {
  const ulak::json::Value* value = ulak::json::GetObjectField(object, key);
  if (!value) {
    if (error) *error = MakeError(ConfigError::kMissingField,
                                  "Required field '" + key + "' is missing");
    return false;
  }
  if (value->type != ulak::json::Type::kString) {
    if (error) *error = MakeError(ConfigError::kInvalidType,
                                  "Field '" + key + "' must be a string");
    return false;
  }
  if (value->string_value.empty()) {
    if (error) *error = MakeError(ConfigError::kInvalidValue,
                                  "Field '" + key + "' must not be empty");
    return false;
  }
  *out = value->string_value;
  return true;
}

bool RequireObjectField(const ulak::json::Value& object,
                        const std::string& key,
                        const ulak::json::Value** out,
                        ConfigResult* error) {
  const ulak::json::Value* value = ulak::json::GetObjectField(object, key);
  if (!value) {
    if (error) *error = MakeError(ConfigError::kMissingField,
                                  "Required object '" + key + "' is missing");
    return false;
  }
  if (value->type != ulak::json::Type::kObject) {
    if (error) *error = MakeError(ConfigError::kInvalidType,
                                  "Field '" + key + "' must be an object");
    return false;
  }
  *out = value;
  return true;
}

// Parses and validates a TCP port number (1–65535).
bool ParsePort(const ulak::json::Value& object,
               const std::string& key,
               int* out,
               ConfigResult* error) {
  const ulak::json::Value* value = ulak::json::GetObjectField(object, key);
  if (!value) {
    if (error) *error = MakeError(ConfigError::kMissingField,
                                  "Required field '" + key + "' is missing");
    return false;
  }
  int port = 0;
  if (!ulak::json::ToInt(*value, &port)) {
    if (error) *error = MakeError(ConfigError::kInvalidType,
                                  "Field '" + key + "' must be an integer");
    return false;
  }
  if (port <= 0 || port > 65535) {
    if (error) *error = MakeError(ConfigError::kInvalidValue,
                                  "Field '" + key + "' must be a valid port (1-65535)");
    return false;
  }
  *out = port;
  return true;
}

// Parses a required integer field with no range restriction.
bool RequireIntField(const ulak::json::Value& object,
                     const std::string& key,
                     int* out,
                     ConfigResult* error) {
  const ulak::json::Value* value = ulak::json::GetObjectField(object, key);
  if (!value) {
    if (error) *error = MakeError(ConfigError::kMissingField,
                                  "Required field '" + key + "' is missing");
    return false;
  }
  int parsed = 0;
  if (!ulak::json::ToInt(*value, &parsed)) {
    if (error) *error = MakeError(ConfigError::kInvalidType,
                                  "Field '" + key + "' must be an integer");
    return false;
  }
  *out = parsed;
  return true;
}

bool OptionalIntField(const ulak::json::Value& object,
                      const std::string& key,
                      int* out,
                      ConfigResult* error) {
  const ulak::json::Value* value = ulak::json::GetObjectField(object, key);
  if (!value) return true;
  int parsed = 0;
  if (!ulak::json::ToInt(*value, &parsed)) {
    if (error) *error = MakeError(ConfigError::kInvalidType,
                                  "Field '" + key + "' must be an integer");
    return false;
  }
  *out = parsed;
  return true;
}

bool OptionalBoolField(const ulak::json::Value& object,
                       const std::string& key,
                       bool* out,
                       ConfigResult* error) {
  const ulak::json::Value* value = ulak::json::GetObjectField(object, key);
  if (!value) return true;
  if (value->type != ulak::json::Type::kBool) {
    if (error) *error = MakeError(ConfigError::kInvalidType,
                                  "Field '" + key + "' must be a boolean");
    return false;
  }
  *out = value->bool_value;
  return true;
}

bool OptionalStringField(const ulak::json::Value& object,
                         const std::string& key,
                         std::string* out,
                         ConfigResult* error) {
  const ulak::json::Value* value = ulak::json::GetObjectField(object, key);
  if (!value) return true;
  if (value->type != ulak::json::Type::kString) {
    if (error) *error = MakeError(ConfigError::kInvalidType,
                                  "Field '" + key + "' must be a string");
    return false;
  }
  *out = value->string_value;
  return true;
}

// Parses an optional double field; missing field is silently skipped.
bool OptionalDoubleField(const ulak::json::Value& object,
                         const std::string& key,
                         double* out) {
  double value = 0.0;
  if (!ulak::json::GetNumberField(object, key, &value)) return true;
  *out = value;
  return true;
}

// ── Composite parsers ────────────────────────────────────────────────────────

// Parses x/y/z double fields from an object into a Vec3Config.
void ParseVec3(const ulak::json::Value& object, Vec3Config* out) {
  OptionalDoubleField(object, "x", &out->x);
  OptionalDoubleField(object, "y", &out->y);
  OptionalDoubleField(object, "z", &out->z);
}

// Parses a TCP endpoint: requires host (string) and port (1-65535).
bool ParseTcpEndpoint(const ulak::json::Value& object,
                      TcpEndpointConfig* out,
                      ConfigResult* error) {
  if (!RequireStringField(object, "host", &out->host, error)) return false;
  if (!ParsePort(object, "port", &out->port, error)) return false;
  return true;
}

// Parses a serial endpoint: requires port (string, e.g. "COM3") and baud (int).
bool ParseSerialEndpoint(const ulak::json::Value& object,
                         SerialEndpointConfig* out,
                         ConfigResult* error) {
  if (!RequireStringField(object, "port", &out->port, error)) return false;
  if (!RequireIntField(object, "baud", &out->baud, error)) return false;
  return true;
}

}  // namespace

// ── Public API ───────────────────────────────────────────────────────────────

ConfigResult LoadConfigFile(const std::filesystem::path& path, AppConfig* out) {
  if (!out) {
    return MakeError(ConfigError::kInvalidValue, "Output config pointer is null");
  }

  std::error_code ec;
  if (!std::filesystem::exists(path, ec)) {
    return MakeError(ConfigError::kMissingFile, "Config file not found: " + path.string());
  }
  if (ec) {
    return MakeError(ConfigError::kUnreadableFile,
                     "Failed to check config file: " + ec.message());
  }
  if (std::filesystem::is_directory(path, ec)) {
    return MakeError(ConfigError::kUnreadableFile,
                     "Config path is a directory: " + path.string());
  }
  if (ec) {
    return MakeError(ConfigError::kUnreadableFile,
                     "Failed to inspect config path: " + ec.message());
  }

  std::ifstream input(path);
  if (!input.good()) {
    return MakeError(ConfigError::kUnreadableFile,
                     "Config file cannot be opened: " + path.string());
  }

  std::ostringstream buffer;
  buffer << input.rdbuf();
  if (input.bad()) {
    return MakeError(ConfigError::kUnreadableFile,
                     "Config file cannot be read: " + path.string());
  }

  const std::string content = buffer.str();
  if (content.empty()) {
    return MakeError(ConfigError::kInvalidJson, "Config file is empty");
  }

  ulak::json::Value root;
  std::string parse_error;
  if (!ulak::json::Parse(content, &root, &parse_error)) {
    return MakeError(ConfigError::kInvalidJson, parse_error);
  }
  if (root.type != ulak::json::Type::kObject) {
    return MakeError(ConfigError::kInvalidType, "Root JSON must be an object");
  }

  AppConfig config;
  ConfigResult error;

  // ── Schema and identity ──────────────────────────────────────────────────

  if (!RequireStringField(root, "schema_version", &config.schema_version, &error)) return error;
  if (config.schema_version != "1.0.0") {
    return MakeError(ConfigError::kUnsupportedSchema,
                     "Unsupported schema_version: " + config.schema_version);
  }
  if (!RequireStringField(root, "instance_name", &config.instance_name, &error)) return error;
  if (!RequireStringField(root, "active_profile", &config.active_profile, &error)) return error;

  // ── Deployment mode ──────────────────────────────────────────────────────

  std::string mode_str;
  if (!RequireStringField(root, "deployment_mode", &mode_str, &error)) return error;
  if (mode_str == "simulation") {
    config.deployment_mode = DeploymentMode::kSimulation;
  } else if (mode_str == "real") {
    config.deployment_mode = DeploymentMode::kReal;
  } else {
    return MakeError(ConfigError::kInvalidValue,
                     "Invalid deployment_mode: '" + mode_str +
                     "' (must be 'simulation' or 'real')");
  }

  // ── ROS2 config (simulation mode transport) ──────────────────────────────

  const ulak::json::Value* ros2 = nullptr;
  if (!RequireObjectField(root, "ros2", &ros2, &error)) return error;

  const ulak::json::Value* bridge = nullptr;
  if (!RequireObjectField(*ros2, "bridge", &bridge, &error)) return error;
  if (!RequireStringField(*bridge, "host", &config.ros2.bridge.host, &error)) return error;
  if (!ParsePort(*bridge, "port", &config.ros2.bridge.port, &error)) return error;

  const ulak::json::Value* topics = nullptr;
  if (!RequireObjectField(*ros2, "topics", &topics, &error)) return error;
  if (!RequireStringField(*topics, "telemetry",
                          &config.ros2.topics.telemetry, &error)) return error;
  if (!RequireStringField(*topics, "vehicle_state",
                          &config.ros2.topics.vehicle_state, &error)) return error;
  if (!RequireStringField(*topics, "mission_state",
                          &config.ros2.topics.mission_state, &error)) return error;
  if (!RequireStringField(*topics, "perception_output",
                          &config.ros2.topics.perception_output, &error)) return error;
  if (!RequireStringField(*topics, "safety_events",
                          &config.ros2.topics.safety_events, &error)) return error;
  if (!RequireStringField(*topics, "camera",
                          &config.ros2.topics.camera, &error)) return error;
  if (!RequireStringField(*topics, "commands",
                          &config.ros2.topics.commands, &error)) return error;

  // ── Network config (real mode transport) ────────────────────────────────

  const ulak::json::Value* network = nullptr;
  if (!RequireObjectField(root, "network", &network, &error)) return error;

  // telemetry_endpoint: MAVLink serial (port is a string, e.g. "COM3")
  const ulak::json::Value* telemetry_ep = nullptr;
  if (!RequireObjectField(*network, "telemetry_endpoint", &telemetry_ep, &error)) return error;
  if (!ParseSerialEndpoint(*telemetry_ep,
                           &config.network.telemetry_endpoint, &error)) return error;

  // companion_endpoint: JSON-over-TCP from Raspberry Pi
  const ulak::json::Value* companion_ep = nullptr;
  if (!RequireObjectField(*network, "companion_endpoint", &companion_ep, &error)) return error;
  if (!ParseTcpEndpoint(*companion_ep,
                        &config.network.companion_endpoint, &error)) return error;

  // command_endpoint: JSON-over-TCP to Raspberry Pi
  const ulak::json::Value* command_ep = nullptr;
  if (!RequireObjectField(*network, "command_endpoint", &command_ep, &error)) return error;
  if (!ParseTcpEndpoint(*command_ep,
                        &config.network.command_endpoint, &error)) return error;

  // stream config
  const ulak::json::Value* stream = nullptr;
  if (!RequireObjectField(*network, "stream", &stream, &error)) return error;
  if (!RequireStringField(*stream, "mode", &config.network.stream.mode, &error)) return error;

  static const std::unordered_set<std::string> kStreamModes = {
      "OFF", "OUTPUTS_ONLY", "COMPRESSED_LIVE", "RAW_DEBUG"};
  if (kStreamModes.find(config.network.stream.mode) == kStreamModes.end()) {
    return MakeError(ConfigError::kInvalidValue,
                     "Invalid network.stream.mode: '" + config.network.stream.mode + "'");
  }
  if (!OptionalStringField(*stream, "host", &config.network.stream.host, &error)) return error;
  if (!OptionalIntField(*stream, "port", &config.network.stream.port, &error)) return error;
  if (!OptionalStringField(*stream, "codec", &config.network.stream.codec, &error)) return error;
  if (!OptionalIntField(*stream, "fps", &config.network.stream.fps, &error)) return error;
  if (!OptionalIntField(*stream, "bitrate_kbps",
                        &config.network.stream.bitrate_kbps, &error)) return error;

  // ── Simulation config (coordinate transform) ─────────────────────────────

  const ulak::json::Value* simulation = ulak::json::GetObjectField(root, "simulation");
  if (simulation && simulation->type == ulak::json::Type::kObject) {
    if (!OptionalStringField(*simulation, "active_transform_id",
                             &config.simulation.active_transform_id, &error)) return error;

    const ulak::json::Value* xform =
        ulak::json::GetObjectField(*simulation, "coordinate_transform");
    if (xform && xform->type == ulak::json::Type::kObject) {
      if (!OptionalBoolField(*xform, "enabled",
                             &config.simulation.coordinate_transform.enabled,
                             &error)) return error;
      if (!OptionalStringField(*xform, "source_frame_id",
                               &config.simulation.coordinate_transform.source_frame_id,
                               &error)) return error;
      if (!OptionalStringField(*xform, "target_frame_id",
                               &config.simulation.coordinate_transform.target_frame_id,
                               &error)) return error;

      const ulak::json::Value* axis_map = ulak::json::GetObjectField(*xform, "axis_map");
      if (axis_map && axis_map->type == ulak::json::Type::kObject) {
        if (!OptionalStringField(*axis_map, "x",
                                 &config.simulation.coordinate_transform.axis_map.x,
                                 &error)) return error;
        if (!OptionalStringField(*axis_map, "y",
                                 &config.simulation.coordinate_transform.axis_map.y,
                                 &error)) return error;
        if (!OptionalStringField(*axis_map, "z",
                                 &config.simulation.coordinate_transform.axis_map.z,
                                 &error)) return error;
      }

      const ulak::json::Value* translation =
          ulak::json::GetObjectField(*xform, "translation_m");
      if (translation && translation->type == ulak::json::Type::kObject) {
        ParseVec3(*translation, &config.simulation.coordinate_transform.translation_m);
      }

      const ulak::json::Value* rotation =
          ulak::json::GetObjectField(*xform, "rotation_deg");
      if (rotation && rotation->type == ulak::json::Type::kObject) {
        ParseVec3(*rotation, &config.simulation.coordinate_transform.rotation_deg);
      }

      const ulak::json::Value* scale = ulak::json::GetObjectField(*xform, "scale");
      if (scale && scale->type == ulak::json::Type::kObject) {
        ParseVec3(*scale, &config.simulation.coordinate_transform.scale);
      }
    }
  }

  // ── Command lifecycle ────────────────────────────────────────────────────

  const ulak::json::Value* lifecycle = ulak::json::GetObjectField(root, "command_lifecycle");
  if (lifecycle && lifecycle->type == ulak::json::Type::kObject) {
    if (!OptionalIntField(*lifecycle, "ack_timeout_ms",
                          &config.command_lifecycle.ack_timeout_ms, &error)) return error;
    if (!OptionalIntField(*lifecycle, "exec_timeout_ms",
                          &config.command_lifecycle.exec_timeout_ms, &error)) return error;
    const ulak::json::Value* retry = ulak::json::GetObjectField(*lifecycle, "retry");
    if (retry && retry->type == ulak::json::Type::kObject) {
      if (!OptionalIntField(*retry, "max_attempts",
                            &config.command_lifecycle.retry.max_attempts, &error)) return error;
      const ulak::json::Value* backoff = ulak::json::GetObjectField(*retry, "backoff_ms");
      if (backoff && backoff->type == ulak::json::Type::kArray) {
        for (const auto& item : backoff->array_value) {
          int delay = 0;
          if (ulak::json::ToInt(item, &delay)) {
            config.command_lifecycle.retry.backoff_ms.push_back(delay);
          }
        }
      }
    }
  }

  // ── Safety ───────────────────────────────────────────────────────────────

  const ulak::json::Value* safety = ulak::json::GetObjectField(root, "safety");
  if (safety && safety->type == ulak::json::Type::kObject) {
    if (!OptionalIntField(*safety, "error_confirm_window_sec",
                          &config.safety.error_confirm_window_sec, &error)) return error;
    if (!OptionalBoolField(*safety, "panic_requires_guard",
                           &config.safety.panic_requires_guard, &error)) return error;
    if (!OptionalIntField(*safety, "panic_lockout_ms",
                          &config.safety.panic_lockout_ms, &error)) return error;
  }

  // ── Logging ──────────────────────────────────────────────────────────────

  const ulak::json::Value* logging = ulak::json::GetObjectField(root, "logging");
  if (logging && logging->type == ulak::json::Type::kObject) {
    if (!OptionalStringField(*logging, "directory",
                             &config.logging.directory, &error)) return error;
    if (!OptionalBoolField(*logging, "ndjson_enabled",
                           &config.logging.ndjson_enabled, &error)) return error;
    if (!OptionalStringField(*logging, "session_rotation",
                             &config.logging.session_rotation, &error)) return error;
  }

  *out = std::move(config);
  ConfigResult ok;
  ok.ok = true;
  ok.error = ConfigError::kNone;
  ok.message = "Config loaded successfully";
  return ok;
}

}  // namespace ulak::core
