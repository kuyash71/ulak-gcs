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

struct ValidationResult {
  bool ok{false};
  ValidationError error{ValidationError::kNone};
  std::string message;
  std::string schema_version;
  std::string active_profile;
};

ValidationResult ValidateConfigFile(const std::filesystem::path& config_path);

const char* ToString(ValidationError error);

}  // namespace ulak::bootstrap
