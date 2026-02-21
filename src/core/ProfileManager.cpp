#include "ProfileManager.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>

#include "JsonUtils.h"

namespace ulak::core {
namespace {

ProfileManagerResult MakeError(ConfigError error, std::string message) {
  ProfileManagerResult result;
  result.ok = false;
  result.error = error;
  result.message = std::move(message);
  return result;
}

ProfileManagerResult MakeOk(std::string message) {
  ProfileManagerResult result;
  result.ok = true;
  result.error = ConfigError::kNone;
  result.message = std::move(message);
  return result;
}

bool RequireObjectField(const ulak::json::Value& object,
                        const std::string& key,
                        const ulak::json::Value** out,
                        ProfileManagerResult* error) {
  const ulak::json::Value* value = ulak::json::GetObjectField(object, key);
  if (!value) {
    if (error) {
      *error = MakeError(ConfigError::kMissingField,
                         "Required object field '" + key + "' is missing");
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

bool RequireStringField(const ulak::json::Value& object,
                        const std::string& key,
                        std::string* out,
                        ProfileManagerResult* error) {
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
                       ProfileManagerResult* error) {
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

bool OptionalPositiveIntField(const ulak::json::Value& object,
                              const std::string& key,
                              int* out,
                              ProfileManagerResult* error) {
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
  if (parsed <= 0) {
    if (error) {
      *error = MakeError(ConfigError::kInvalidValue,
                         "Field '" + key + "' must be > 0");
    }
    return false;
  }
  *out = parsed;
  return true;
}

bool ParsePolicyRule(const ulak::json::Value& value,
                     PolicyMapping* out,
                     ProfileManagerResult* error) {
  if (value.type != ulak::json::Type::kObject) {
    if (error) {
      *error = MakeError(ConfigError::kInvalidType, "Policy mapping entry must be an object");
    }
    return false;
  }

  PolicyMapping mapping;
  std::string severity_text;
  std::string action_text;

  if (!RequireStringField(value, "event_code", &mapping.event_code, error)) {
    return false;
  }
  if (!RequireStringField(value, "severity", &severity_text, error)) {
    return false;
  }
  if (!RequireStringField(value, "action", &action_text, error)) {
    return false;
  }
  if (!ParseSeverityLevel(severity_text, &mapping.severity)) {
    if (error) {
      *error = MakeError(ConfigError::kInvalidValue,
                         "Unsupported severity value: " + severity_text);
    }
    return false;
  }
  if (!ParseSafetyAction(action_text, &mapping.action)) {
    if (error) {
      *error = MakeError(ConfigError::kInvalidValue,
                         "Unsupported action value: " + action_text);
    }
    return false;
  }

  std::string timeout_action_text;
  const ulak::json::Value* timeout_action = ulak::json::GetObjectField(value, "timeout_action");
  if (timeout_action) {
    if (timeout_action->type != ulak::json::Type::kString) {
      if (error) {
        *error = MakeError(ConfigError::kInvalidType,
                           "Field 'timeout_action' must be a string");
      }
      return false;
    }
    timeout_action_text = timeout_action->string_value;
    SafetyAction parsed = SafetyAction::kNotifyOnly;
    if (!ParseSafetyAction(timeout_action_text, &parsed)) {
      if (error) {
        *error = MakeError(ConfigError::kInvalidValue,
                           "Unsupported timeout_action value: " + timeout_action_text);
      }
      return false;
    }
    mapping.timeout_action = parsed;
  }

  *out = std::move(mapping);
  return true;
}

bool ParseUnknownEventPolicy(const ulak::json::Value& value,
                             UnknownEventPolicy* out,
                             ProfileManagerResult* error) {
  if (!out) {
    if (error) {
      *error = MakeError(ConfigError::kInvalidValue, "Unknown event policy output is null");
    }
    return false;
  }
  if (value.type != ulak::json::Type::kObject) {
    if (error) {
      *error = MakeError(ConfigError::kInvalidType, "unknown_event policy must be an object");
    }
    return false;
  }

  std::string severity_text;
  std::string action_text;
  if (!RequireStringField(value, "severity", &severity_text, error)) {
    return false;
  }
  if (!RequireStringField(value, "action", &action_text, error)) {
    return false;
  }

  UnknownEventPolicy unknown;
  if (!ParseSeverityLevel(severity_text, &unknown.severity)) {
    if (error) {
      *error = MakeError(ConfigError::kInvalidValue,
                         "Unsupported unknown_event severity: " + severity_text);
    }
    return false;
  }
  if (!ParseSafetyAction(action_text, &unknown.action)) {
    if (error) {
      *error = MakeError(ConfigError::kInvalidValue,
                         "Unsupported unknown_event action: " + action_text);
    }
    return false;
  }

  const ulak::json::Value* timeout_action = ulak::json::GetObjectField(value, "timeout_action");
  if (timeout_action) {
    if (timeout_action->type != ulak::json::Type::kString) {
      if (error) {
        *error = MakeError(ConfigError::kInvalidType,
                           "Field 'unknown_event.timeout_action' must be a string");
      }
      return false;
    }
    SafetyAction timeout_parsed = SafetyAction::kNotifyOnly;
    if (!ParseSafetyAction(timeout_action->string_value, &timeout_parsed)) {
      if (error) {
        *error = MakeError(ConfigError::kInvalidValue,
                           "Unsupported unknown_event timeout_action: " +
                               timeout_action->string_value);
      }
      return false;
    }
    unknown.timeout_action = timeout_parsed;
  }

  *out = std::move(unknown);
  return true;
}

std::string JoinProfileIds(const std::vector<std::string>& profile_ids) {
  std::string joined;
  for (size_t i = 0; i < profile_ids.size(); ++i) {
    if (i > 0) {
      joined += ",";
    }
    joined += profile_ids[i];
  }
  return joined;
}

}  // namespace

const char* ToString(SeverityLevel level) {
  switch (level) {
    case SeverityLevel::kWarn:
      return "WARN";
    case SeverityLevel::kError:
      return "ERROR";
    case SeverityLevel::kCritical:
      return "CRITICAL";
  }
  return "WARN";
}

const char* ToString(SafetyAction action) {
  switch (action) {
    case SafetyAction::kNotifyOnly:
      return "NOTIFY_ONLY";
    case SafetyAction::kRequestConfirmation:
      return "REQUEST_CONFIRMATION";
    case SafetyAction::kPanicRtl:
      return "PANIC_RTL";
    case SafetyAction::kHoldPosition:
      return "HOLD_POSITION";
    case SafetyAction::kRejectCommand:
      return "REJECT_COMMAND";
    case SafetyAction::kStopMission:
      return "STOP_MISSION";
  }
  return "NOTIFY_ONLY";
}

bool ParseSeverityLevel(const std::string& value, SeverityLevel* out) {
  if (!out) {
    return false;
  }
  if (value == "WARN") {
    *out = SeverityLevel::kWarn;
    return true;
  }
  if (value == "ERROR") {
    *out = SeverityLevel::kError;
    return true;
  }
  if (value == "CRITICAL") {
    *out = SeverityLevel::kCritical;
    return true;
  }
  return false;
}

bool ParseSafetyAction(const std::string& value, SafetyAction* out) {
  if (!out) {
    return false;
  }
  if (value == "NOTIFY_ONLY") {
    *out = SafetyAction::kNotifyOnly;
    return true;
  }
  if (value == "REQUEST_CONFIRMATION") {
    *out = SafetyAction::kRequestConfirmation;
    return true;
  }
  if (value == "PANIC_RTL") {
    *out = SafetyAction::kPanicRtl;
    return true;
  }
  if (value == "HOLD_POSITION") {
    *out = SafetyAction::kHoldPosition;
    return true;
  }
  if (value == "REJECT_COMMAND") {
    *out = SafetyAction::kRejectCommand;
    return true;
  }
  if (value == "STOP_MISSION") {
    *out = SafetyAction::kStopMission;
    return true;
  }
  return false;
}

ProfileManagerResult ProfileManager::LoadSingleProfile(const std::filesystem::path& path,
                                                       ProfileData* out) const {
  if (!out) {
    return MakeError(ConfigError::kInvalidValue, "Output profile pointer is null");
  }

  std::error_code ec;
  if (!std::filesystem::exists(path, ec)) {
    return MakeError(ConfigError::kMissingFile, "Profile file not found: " + path.string());
  }
  if (ec) {
    return MakeError(ConfigError::kUnreadableFile,
                     "Failed to check profile file: " + ec.message());
  }
  if (std::filesystem::is_directory(path, ec)) {
    return MakeError(ConfigError::kUnreadableFile, "Profile path is a directory: " + path.string());
  }
  if (ec) {
    return MakeError(ConfigError::kUnreadableFile,
                     "Failed to inspect profile path: " + ec.message());
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

  ulak::json::Value root;
  std::string parse_error;
  if (!ulak::json::Parse(content, &root, &parse_error)) {
    return MakeError(ConfigError::kInvalidJson, parse_error);
  }
  if (root.type != ulak::json::Type::kObject) {
    return MakeError(ConfigError::kInvalidType, "Profile JSON root must be an object");
  }

  ProfileManagerResult error;
  ProfileData parsed;
  if (!RequireStringField(root, "schema_version", &parsed.schema_version, &error)) {
    return error;
  }
  if (parsed.schema_version != "1.0.0") {
    return MakeError(ConfigError::kUnsupportedSchema,
                     "Unsupported profile schema_version: " + parsed.schema_version);
  }
  if (!RequireStringField(root, "profile_id", &parsed.profile_id, &error)) {
    return error;
  }
  if (!RequireStringField(root, "display_name", &parsed.display_name, &error)) {
    return error;
  }
  if (!OptionalBoolField(root, "protected", &parsed.protected_profile, &error)) {
    return error;
  }

  const ulak::json::Value* policy = nullptr;
  if (!RequireObjectField(root, "policy", &policy, &error)) {
    return error;
  }
  const ulak::json::Value* defaults = nullptr;
  if (!RequireObjectField(*policy, "defaults", &defaults, &error)) {
    return error;
  }
  const ulak::json::Value* unknown_event = nullptr;
  if (!RequireObjectField(*defaults, "unknown_event", &unknown_event, &error)) {
    return error;
  }

  if (!ParseUnknownEventPolicy(*unknown_event, &parsed.unknown_event, &error)) {
    return error;
  }

  const ulak::json::Value* mappings = ulak::json::GetObjectField(*policy, "mappings");
  if (!mappings) {
    return MakeError(ConfigError::kMissingField, "Required field 'policy.mappings' is missing");
  }
  if (mappings->type != ulak::json::Type::kArray) {
    return MakeError(ConfigError::kInvalidType, "Field 'policy.mappings' must be an array");
  }
  for (const auto& item : mappings->array_value) {
    PolicyMapping mapping;
    if (!ParsePolicyRule(item, &mapping, &error)) {
      return error;
    }
    parsed.mappings.push_back(std::move(mapping));
  }

  const ulak::json::Value* timers = ulak::json::GetObjectField(root, "timers");
  if (timers) {
    if (timers->type != ulak::json::Type::kObject) {
      return MakeError(ConfigError::kInvalidType, "Field 'timers' must be an object");
    }
    if (!OptionalPositiveIntField(*timers,
                                  "error_confirm_window_sec",
                                  &parsed.error_confirm_window_sec,
                                  &error)) {
      return error;
    }
  }

  const ulak::json::Value* limits = ulak::json::GetObjectField(root, "limits");
  if (limits) {
    if (limits->type != ulak::json::Type::kObject) {
      return MakeError(ConfigError::kInvalidType, "Field 'limits' must be an object");
    }
    if (!OptionalPositiveIntField(*limits,
                                  "command_rate_limit_per_sec",
                                  &parsed.command_rate_limit_per_sec,
                                  &error)) {
      return error;
    }
  }

  if (parsed.profile_id == "default") {
    parsed.protected_profile = true;
  }

  *out = std::move(parsed);
  return MakeOk("Profile loaded successfully");
}

ProfileManagerResult ProfileManager::LoadProfiles(const std::filesystem::path& profiles_directory,
                                                  const std::string& initial_active_profile) {
  profiles_.clear();
  active_profile_id_.clear();
  audit_log_.clear();

  const std::vector<std::string> required_profiles = {"default", "safe", "aggressive"};
  for (const std::string& profile_name : required_profiles) {
    ProfileData profile;
    const auto path = profiles_directory / (profile_name + ".json");
    ProfileManagerResult load_result = LoadSingleProfile(path, &profile);
    if (!load_result.ok) {
      return load_result;
    }
    if (profile.profile_id != profile_name) {
      return MakeError(ConfigError::kInvalidValue,
                       "Profile id mismatch in " + path.string() +
                           ": expected '" + profile_name + "', got '" + profile.profile_id + "'");
    }
    profiles_[profile.profile_id] = std::move(profile);
  }

  const auto selected_it = profiles_.find(initial_active_profile);
  if (selected_it == profiles_.end()) {
    active_profile_id_ = "default";
    AddAudit("active_profile_fallback:" + initial_active_profile + "->default");
  } else {
    active_profile_id_ = initial_active_profile;
  }

  AddAudit("profiles_loaded:" + JoinProfileIds(GetProfileIds()));
  AddAudit("active_profile_set:" + active_profile_id_);
  return MakeOk("Profiles loaded successfully");
}

bool ProfileManager::SwitchActiveProfile(const std::string& profile_id, std::string* reason) {
  if (profiles_.find(profile_id) == profiles_.end()) {
    if (reason) {
      *reason = "Profile not loaded: " + profile_id;
    }
    return false;
  }
  active_profile_id_ = profile_id;
  AddAudit("active_profile_switched:" + profile_id);
  return true;
}

bool ProfileManager::DeleteProfileInUiWorkflow(const std::string& profile_id, std::string* reason) {
  const auto it = profiles_.find(profile_id);
  if (it == profiles_.end()) {
    if (reason) {
      *reason = "Profile not found: " + profile_id;
    }
    return false;
  }

  if (it->second.protected_profile || profile_id == "default") {
    if (reason) {
      *reason = "Protected profile cannot be deleted from UI workflow: " + profile_id;
    }
    AddAudit("profile_delete_rejected:" + profile_id);
    return false;
  }

  profiles_.erase(it);
  AddAudit("profile_deleted:" + profile_id);
  if (active_profile_id_ == profile_id) {
    active_profile_id_ = "default";
    AddAudit("active_profile_fallback:deleted->default");
  }
  return true;
}

const ProfileData* ProfileManager::GetActiveProfile() const {
  const auto it = profiles_.find(active_profile_id_);
  if (it == profiles_.end()) {
    return nullptr;
  }
  return &it->second;
}

const ProfileData* ProfileManager::FindProfile(const std::string& profile_id) const {
  const auto it = profiles_.find(profile_id);
  if (it == profiles_.end()) {
    return nullptr;
  }
  return &it->second;
}

std::vector<std::string> ProfileManager::GetProfileIds() const {
  std::vector<std::string> ids;
  ids.reserve(profiles_.size());
  for (const auto& item : profiles_) {
    ids.push_back(item.first);
  }
  std::sort(ids.begin(), ids.end());
  return ids;
}

void ProfileManager::AddAudit(std::string entry) {
  audit_log_.push_back(std::move(entry));
}

}  // namespace ulak::core
