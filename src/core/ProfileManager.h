#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "ConfigLoader.h"

namespace ulak::core {

enum class SeverityLevel {
  kWarn = 0,
  kError,
  kCritical
};

enum class SafetyAction {
  kNotifyOnly = 0,
  kRequestConfirmation,
  kPanicRtl,
  kHoldPosition,
  kRejectCommand,
  kStopMission
};

const char* ToString(SeverityLevel level);
const char* ToString(SafetyAction action);

bool ParseSeverityLevel(const std::string& value, SeverityLevel* out);
bool ParseSafetyAction(const std::string& value, SafetyAction* out);

struct PolicyMapping {
  std::string event_code;
  SeverityLevel severity{SeverityLevel::kWarn};
  SafetyAction action{SafetyAction::kNotifyOnly};
  std::optional<SafetyAction> timeout_action;
};

struct UnknownEventPolicy {
  SeverityLevel severity{SeverityLevel::kError};
  SafetyAction action{SafetyAction::kRequestConfirmation};
  std::optional<SafetyAction> timeout_action;
};

struct ProfileData {
  std::string schema_version;
  std::string profile_id;
  std::string display_name;
  bool protected_profile{false};

  UnknownEventPolicy unknown_event;
  std::vector<PolicyMapping> mappings;

  int error_confirm_window_sec{5};
  int command_rate_limit_per_sec{0};
};

struct ProfileManagerResult {
  bool ok{false};
  ConfigError error{ConfigError::kNone};
  std::string message;
};

class ProfileManager {
 public:
  ProfileManager() = default;

  ProfileManagerResult LoadProfiles(const std::filesystem::path& profiles_directory,
                                    const std::string& initial_active_profile);
  bool SwitchActiveProfile(const std::string& profile_id, std::string* reason = nullptr);
  bool DeleteProfileInUiWorkflow(const std::string& profile_id, std::string* reason = nullptr);

  const ProfileData* GetActiveProfile() const;
  const ProfileData* FindProfile(const std::string& profile_id) const;
  std::vector<std::string> GetProfileIds() const;

  const std::vector<std::string>& audit_log() const {
    return audit_log_;
  }

 private:
  ProfileManagerResult LoadSingleProfile(const std::filesystem::path& path, ProfileData* out) const;
  void AddAudit(std::string entry);

  std::unordered_map<std::string, ProfileData> profiles_;
  std::string active_profile_id_;
  std::vector<std::string> audit_log_;
};

}  // namespace ulak::core
