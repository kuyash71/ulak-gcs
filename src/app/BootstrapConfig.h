#pragma once

#include <filesystem>
#include <string>

namespace ulak::bootstrap {

enum class ValidationError {
  kNone = 0,
  kMissingFile,
  kUnreadableFile,
  kInvalidJson,
  kMissingRequiredField,
  kInvalidFieldType,
  kInvalidFieldValue
};

// Summary of config validation status used by bootstrap and tests.
struct ValidationResult {
  bool ok{false};
  ValidationError error{ValidationError::kNone};
  std::string message;
  std::string schema_version;
  std::string active_profile;
};

// Validate config file presence, JSON sanity, and required fields.
ValidationResult ValidateConfigFile(const std::filesystem::path& config_path);

// Stable string for logs/tests.
const char* ToString(ValidationError error);

}  // namespace ulak::bootstrap
