#pragma once

#include <filesystem>
#include <string>

#include "AppConfig.h"

namespace ulak::core {

enum class ConfigError {
  kNone = 0,
  kMissingFile,
  kUnreadableFile,
  kInvalidJson,
  kMissingField,
  kInvalidType,
  kInvalidValue,
  kUnsupportedSchema
};

struct ConfigResult {
  bool ok{false};
  ConfigError error{ConfigError::kNone};
  std::string message;
};

// Loads config from disk and validates required fields.
ConfigResult LoadConfigFile(const std::filesystem::path& path, AppConfig* out);

}  // namespace ulak::core
