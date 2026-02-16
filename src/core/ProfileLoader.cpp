#include "ProfileLoader.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include "JsonUtils.h"

namespace ulak::core {
namespace {

ProfileLoadResult MakeError(ConfigError error, std::string message) {
  ProfileLoadResult result;
  result.ok = false;
  result.error = error;
  result.message = std::move(message);
  return result;
}

bool RequireStringField(const ulak::json::Value& object,
                        const std::string& key,
                        std::string* out,
                        ProfileLoadResult* error) {
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

bool OptionalBoolField(const ulak::json::Value& object,
                       const std::string& key,
                       bool* out,
                       ProfileLoadResult* error) {
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

ProfileLoadResult ParseProfileContent(const std::string& content, ProfileConfig* out) {
  ulak::json::Value root;
  std::string parse_error;
  if (!ulak::json::Parse(content, &root, &parse_error)) {
    return MakeError(ConfigError::kInvalidJson, parse_error);
  }
  if (root.type != ulak::json::Type::kObject) {
    return MakeError(ConfigError::kInvalidType, "Root JSON must be an object");
  }

  ProfileConfig profile;
  ProfileLoadResult error;
  if (!RequireStringField(root, "schema_version", &profile.schema_version, &error)) {
    return error;
  }
  if (!RequireStringField(root, "profile_id", &profile.profile_id, &error)) {
    return error;
  }
  if (!RequireStringField(root, "display_name", &profile.display_name, &error)) {
    return error;
  }
  if (!OptionalBoolField(root, "protected", &profile.protected_profile, &error)) {
    return error;
  }

  *out = std::move(profile);
  ProfileLoadResult ok;
  ok.ok = true;
  ok.error = ConfigError::kNone;
  ok.message = "Profile loaded successfully";
  return ok;
}

ProfileLoadResult LoadProfileContentFromFile(const std::filesystem::path& path,
                                             ProfileConfig* out) {
  std::error_code ec;
  if (!std::filesystem::exists(path, ec)) {
    return MakeError(ConfigError::kMissingFile, "Profile file not found: " + path.string());
  }
  if (ec) {
    return MakeError(ConfigError::kUnreadableFile, "Failed to check profile file: " + ec.message());
  }
  if (std::filesystem::is_directory(path, ec)) {
    return MakeError(ConfigError::kUnreadableFile, "Profile path is a directory: " + path.string());
  }
  if (ec) {
    return MakeError(ConfigError::kUnreadableFile, "Failed to inspect profile path: " + ec.message());
  }

  std::ifstream input(path);
  if (!input.good()) {
    return MakeError(ConfigError::kUnreadableFile, "Profile file cannot be opened: " + path.string());
  }

  std::ostringstream buffer;
  buffer << input.rdbuf();
  if (input.bad()) {
    return MakeError(ConfigError::kUnreadableFile, "Profile file cannot be read: " + path.string());
  }

  const std::string content = buffer.str();
  if (content.empty()) {
    return MakeError(ConfigError::kInvalidJson, "Profile file is empty");
  }

  return ParseProfileContent(content, out);
}

}  // namespace

ProfileLoadResult LoadProfileFile(const std::filesystem::path& path,
                                  const std::filesystem::path& fallback_path,
                                  ProfileConfig* out) {
  if (!out) {
    return MakeError(ConfigError::kInvalidValue, "Output profile pointer is null");
  }

  ProfileConfig profile;
  ProfileLoadResult result = LoadProfileContentFromFile(path, &profile);
  if (!result.ok) {
    return result;
  }

  if (profile.schema_version != "1.0.0") {
    ProfileConfig fallback;
    ProfileLoadResult fallback_result = LoadProfileContentFromFile(fallback_path, &fallback);
    if (!fallback_result.ok) {
      return MakeError(ConfigError::kUnsupportedSchema,
                       "Unsupported schema_version: " + profile.schema_version +
                       " and fallback failed");
    }
    fallback_result.ok = true;
    fallback_result.used_fallback = true;
    fallback_result.error = ConfigError::kUnsupportedSchema;
    fallback_result.message = "Unsupported schema_version: " + profile.schema_version +
                              "; fallback profile loaded";
    *out = std::move(fallback);
    return fallback_result;
  }

  *out = std::move(profile);
  result.ok = true;
  result.used_fallback = false;
  result.error = ConfigError::kNone;
  result.message = "Profile loaded successfully";
  return result;
}

}  // namespace ulak::core
