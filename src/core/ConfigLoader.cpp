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

bool RequireStringField(const ulak::json::Value& object,
                        const std::string& key,
                        std::string* out,
                        ConfigResult* error) {
  const ulak::json::Value* value = ulak::json::GetObjectField(object, key);
  if (!value) {
    if (error) {
      *error = MakeError(ConfigError::kMissingField,
                         "Required field '" + key + "' is missing");
    }
    return false;
  }
  if (value->type != ulak::json::Type::kString) {
    if (error) {
      *error = MakeError(ConfigError::kInvalidType,
                         "Field '" + key + "' must be a string");
    }
    return false;
  }
  if (value->string_value.empty()) {
    if (error) {
      *error = MakeError(ConfigError::kInvalidValue,
                         "Field '" + key + "' must not be empty");
    }
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
    if (error) {
      *error = MakeError(ConfigError::kMissingField,
                         "Required object '" + key + "' is missing");
    }
    return false;
  }
  if (value->type != ulak::json::Type::kObject) {
    if (error) {
      *error = MakeError(ConfigError::kInvalidType,
                         "Field '" + key + "' must be an object");
    }
    return false;
  }
  *out = value;
  return true;
}

bool ParsePort(const ulak::json::Value& object,
               const std::string& key,
               int* out,
               ConfigResult* error) {
  const ulak::json::Value* value = ulak::json::GetObjectField(object, key);
  if (!value) {
    if (error) {
      *error = MakeError(ConfigError::kMissingField,
                         "Required field '" + key + "' is missing");
    }
    return false;
  }
  int port = 0;
  if (!ulak::json::ToInt(*value, &port)) {
    if (error) {
      *error = MakeError(ConfigError::kInvalidType,
                         "Field '" + key + "' must be an integer");
    }
    return false;
  }
  if (port <= 0 || port > 65535) {
    if (error) {
      *error = MakeError(ConfigError::kInvalidValue,
                         "Field '" + key + "' must be a valid port number");
    }
    return false;
  }
  *out = port;
  return true;
}

bool ParseEndpoint(const ulak::json::Value& object,
                   const std::string& key,
                   EndpointConfig* out,
                   ConfigResult* error) {
  const ulak::json::Value* endpoint = nullptr;
  if (!RequireObjectField(object, key, &endpoint, error)) {
    return false;
  }
  if (!RequireStringField(*endpoint, "transport", &out->transport, error)) {
    return false;
  }
  if (!RequireStringField(*endpoint, "host", &out->host, error)) {
    return false;
  }
  if (!ParsePort(*endpoint, "port", &out->port, error)) {
    return false;
  }
  return true;
}

bool OptionalIntField(const ulak::json::Value& object,
                      const std::string& key,
                      int* out,
                      ConfigResult* error) {
  const ulak::json::Value* value = ulak::json::GetObjectField(object, key);
  if (!value) {
    return true;
  }
  int parsed = 0;
  if (!ulak::json::ToInt(*value, &parsed)) {
    if (error) {
      *error = MakeError(ConfigError::kInvalidType,
                         "Field '" + key + "' must be an integer");
    }
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
  if (!value) {
    return true;
  }
  if (value->type != ulak::json::Type::kBool) {
    if (error) {
      *error = MakeError(ConfigError::kInvalidType,
                         "Field '" + key + "' must be a boolean");
    }
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
  if (!value) {
    return true;
  }
  if (value->type != ulak::json::Type::kString) {
    if (error) {
      *error = MakeError(ConfigError::kInvalidType,
                         "Field '" + key + "' must be a string");
    }
    return false;
  }
  *out = value->string_value;
  return true;
}

}  // namespace

ConfigResult LoadConfigFile(const std::filesystem::path& path, AppConfig* out) {
  if (!out) {
    return MakeError(ConfigError::kInvalidValue, "Output config pointer is null");
  }

  std::error_code ec;
  if (!std::filesystem::exists(path, ec)) {
    return MakeError(ConfigError::kMissingFile, "Config file not found: " + path.string());
  }
  if (ec) {
    return MakeError(ConfigError::kUnreadableFile, "Failed to check config file: " + ec.message());
  }
  if (std::filesystem::is_directory(path, ec)) {
    return MakeError(ConfigError::kUnreadableFile, "Config path is a directory: " + path.string());
  }
  if (ec) {
    return MakeError(ConfigError::kUnreadableFile, "Failed to inspect config path: " + ec.message());
  }

  std::ifstream input(path);
  if (!input.good()) {
    return MakeError(ConfigError::kUnreadableFile, "Config file cannot be opened: " + path.string());
  }

  std::ostringstream buffer;
  buffer << input.rdbuf();
  if (input.bad()) {
    return MakeError(ConfigError::kUnreadableFile, "Config file cannot be read: " + path.string());
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

  if (!RequireStringField(root, "schema_version", &config.schema_version, &error)) {
    return error;
  }
  if (config.schema_version != "1.0.0") {
    return MakeError(ConfigError::kUnsupportedSchema,
                     "Unsupported schema_version: " + config.schema_version);
  }
  if (!RequireStringField(root, "instance_name", &config.instance_name, &error)) {
    return error;
  }
  if (!RequireStringField(root, "active_profile", &config.active_profile, &error)) {
    return error;
  }

  const ulak::json::Value* telemetry = nullptr;
  if (!RequireObjectField(root, "telemetry", &telemetry, &error)) {
    return error;
  }
  if (!ParseEndpoint(*telemetry, "vehicle_endpoint", &config.telemetry.vehicle_endpoint, &error)) {
    return error;
  }
  if (!ParseEndpoint(*telemetry, "simulator_endpoint", &config.telemetry.simulator_endpoint, &error)) {
    return error;
  }
  if (!OptionalIntField(*telemetry, "health_interval_ms", &config.telemetry.health_interval_ms, &error)) {
    return error;
  }

  const ulak::json::Value* companion = nullptr;
  if (!RequireObjectField(root, "companion", &companion, &error)) {
    return error;
  }
  if (!ParseEndpoint(*companion, "endpoint", &config.companion.endpoint, &error)) {
    return error;
  }
  if (!ParseEndpoint(*companion, "command_endpoint", &config.companion.command_endpoint, &error)) {
    return error;
  }

  const ulak::json::Value* stream = nullptr;
  if (!RequireObjectField(root, "stream", &stream, &error)) {
    return error;
  }
  if (!RequireStringField(*stream, "mode", &config.stream.mode, &error)) {
    return error;
  }
  const std::unordered_set<std::string> kStreamModes = {
      "OFF", "OUTPUTS_ONLY", "COMPRESSED_LIVE", "RAW_DEBUG"};
  if (kStreamModes.find(config.stream.mode) == kStreamModes.end()) {
    return MakeError(ConfigError::kInvalidValue,
                     "Invalid stream.mode: " + config.stream.mode);
  }

  const ulak::json::Value* compressed = ulak::json::GetObjectField(*stream, "compressed_live");
  if (compressed && compressed->type == ulak::json::Type::kObject) {
    if (!OptionalIntField(*compressed, "fps", &config.stream.fps, &error)) {
      return error;
    }
    if (!OptionalIntField(*compressed, "bitrate_kbps", &config.stream.bitrate_kbps, &error)) {
      return error;
    }
  }
  if (!OptionalBoolField(*stream, "raw_debug_enabled", &config.stream.raw_debug_enabled, &error)) {
    return error;
  }

  const ulak::json::Value* lifecycle = ulak::json::GetObjectField(root, "command_lifecycle");
  if (lifecycle && lifecycle->type == ulak::json::Type::kObject) {
    if (!OptionalIntField(*lifecycle, "ack_timeout_ms", &config.command_lifecycle.ack_timeout_ms, &error)) {
      return error;
    }
    if (!OptionalIntField(*lifecycle, "exec_timeout_ms", &config.command_lifecycle.exec_timeout_ms, &error)) {
      return error;
    }
    const ulak::json::Value* retry = ulak::json::GetObjectField(*lifecycle, "retry");
    if (retry && retry->type == ulak::json::Type::kObject) {
      if (!OptionalIntField(*retry, "max_attempts", &config.command_lifecycle.retry.max_attempts, &error)) {
        return error;
      }
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

  const ulak::json::Value* safety = ulak::json::GetObjectField(root, "safety");
  if (safety && safety->type == ulak::json::Type::kObject) {
    if (!OptionalIntField(*safety, "error_confirm_window_sec",
                          &config.safety.error_confirm_window_sec, &error)) {
      return error;
    }
    if (!OptionalBoolField(*safety, "panic_requires_guard",
                           &config.safety.panic_requires_guard, &error)) {
      return error;
    }
    if (!OptionalIntField(*safety, "panic_lockout_ms",
                          &config.safety.panic_lockout_ms, &error)) {
      return error;
    }
  }

  const ulak::json::Value* logging = ulak::json::GetObjectField(root, "logging");
  if (logging && logging->type == ulak::json::Type::kObject) {
    if (!OptionalStringField(*logging, "directory", &config.logging.directory, &error)) {
      return error;
    }
    if (!OptionalBoolField(*logging, "ndjson_enabled", &config.logging.ndjson_enabled, &error)) {
      return error;
    }
    if (!OptionalStringField(*logging, "session_rotation", &config.logging.session_rotation, &error)) {
      return error;
    }
  }

  *out = std::move(config);
  ConfigResult ok;
  ok.ok = true;
  ok.error = ConfigError::kNone;
  ok.message = "Config loaded successfully";
  return ok;
}

}  // namespace ulak::core
