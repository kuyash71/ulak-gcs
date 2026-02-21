#include "ExceptionClassifier.h"
#include "ProfileManager.h"

#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace {

std::filesystem::path ResolveRepoRelative(const std::filesystem::path& relative_path) {
  if (!relative_path.is_relative()) {
    return relative_path;
  }
  std::error_code error_code;
  std::filesystem::path current = std::filesystem::current_path(error_code);
  if (error_code) {
    return relative_path;
  }
  for (int depth = 0; depth < 6; ++depth) {
    const auto candidate = current / relative_path;
    if (std::filesystem::exists(candidate, error_code) && !error_code) {
      return candidate;
    }
    if (current == current.root_path()) {
      break;
    }
    current = current.parent_path();
  }
  return relative_path;
}

bool Expect(bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "[test] " << message << '\n';
    return false;
  }
  return true;
}

struct ExpectedRule {
  std::string event_code;
  ulak::core::SeverityLevel severity;
  ulak::core::SafetyAction action;
  std::optional<ulak::core::SafetyAction> timeout_action;
  bool used_default_mapping{false};
  int confirm_window_sec{0};
};

bool ValidateRule(ulak::core::ExceptionClassifier* classifier, const ExpectedRule& expected) {
  const auto result = classifier->ClassifyEvent(expected.event_code);
  if (!Expect(result.ok, "Expected classification success for " + expected.event_code)) {
    return false;
  }
  if (!Expect(result.severity == expected.severity,
              "Unexpected severity for " + expected.event_code)) {
    return false;
  }
  if (!Expect(result.action == expected.action,
              "Unexpected action for " + expected.event_code)) {
    return false;
  }
  if (!Expect(result.used_default_mapping == expected.used_default_mapping,
              "Unexpected default mapping flag for " + expected.event_code)) {
    return false;
  }
  if (!Expect(result.confirm_window_sec == expected.confirm_window_sec,
              "Unexpected confirm window for " + expected.event_code)) {
    return false;
  }

  if (!expected.timeout_action.has_value()) {
    return Expect(!result.timeout_action.has_value(),
                  "Expected no timeout action for " + expected.event_code);
  }

  if (!Expect(result.timeout_action.has_value(),
              "Expected timeout action for " + expected.event_code)) {
    return false;
  }
  return Expect(result.timeout_action.value() == expected.timeout_action.value(),
                "Unexpected timeout action for " + expected.event_code);
}

bool VerifyProfileMappings(ulak::core::ProfileManager* profile_manager,
                           ulak::core::ExceptionClassifier* classifier,
                           const std::string& profile_id,
                           int expected_confirm_window_sec,
                           const std::vector<ExpectedRule>& rules) {
  std::string switch_error;
  if (!Expect(profile_manager->SwitchActiveProfile(profile_id, &switch_error),
              "Failed to switch profile " + profile_id + ": " + switch_error)) {
    return false;
  }

  const auto* active = profile_manager->GetActiveProfile();
  if (!Expect(active != nullptr, "Expected active profile after switch: " + profile_id)) {
    return false;
  }
  if (!Expect(active->profile_id == profile_id, "Wrong active profile id after switch")) {
    return false;
  }
  if (!Expect(active->error_confirm_window_sec == expected_confirm_window_sec,
              "Wrong profile timer for " + profile_id)) {
    return false;
  }

  for (const auto& rule : rules) {
    if (!ValidateRule(classifier, rule)) {
      return false;
    }
  }

  return true;
}

bool TestProfileManagerAndClassifierMappings() {
  ulak::core::ProfileManager profile_manager;
  const auto profiles_dir = ResolveRepoRelative("config/profiles");
  const auto load = profile_manager.LoadProfiles(profiles_dir, "default");
  if (!Expect(load.ok, "Expected profile loading to succeed: " + load.message)) {
    return false;
  }

  const auto profile_ids = profile_manager.GetProfileIds();
  if (!Expect(profile_ids.size() == 3, "Expected 3 profiles to be loaded")) {
    return false;
  }
  if (!Expect(profile_manager.GetActiveProfile() != nullptr, "Expected active profile")) {
    return false;
  }
  if (!Expect(profile_manager.GetActiveProfile()->profile_id == "default",
              "Expected default profile to be active")) {
    return false;
  }

  ulak::core::ExceptionClassifier classifier(&profile_manager);

  const std::vector<ExpectedRule> default_rules = {
      {"TELEMETRY_LOSS",
       ulak::core::SeverityLevel::kError,
       ulak::core::SafetyAction::kRequestConfirmation,
       ulak::core::SafetyAction::kPanicRtl,
       false,
       5},
      {"CC_LINK_LOSS",
       ulak::core::SeverityLevel::kError,
       ulak::core::SafetyAction::kRequestConfirmation,
       ulak::core::SafetyAction::kPanicRtl,
       false,
       5},
      {"STREAM_LOSS",
       ulak::core::SeverityLevel::kWarn,
       ulak::core::SafetyAction::kNotifyOnly,
       std::nullopt,
       false,
       5},
      {"VISION_LOST",
       ulak::core::SeverityLevel::kWarn,
       ulak::core::SafetyAction::kNotifyOnly,
       std::nullopt,
       false,
       5},
      {"LOW_BATTERY",
       ulak::core::SeverityLevel::kCritical,
       ulak::core::SafetyAction::kPanicRtl,
       std::nullopt,
       false,
       5},
      {"INVALID_OPERATOR_ACTION",
       ulak::core::SeverityLevel::kError,
       ulak::core::SafetyAction::kRejectCommand,
       std::nullopt,
       false,
       5},
      {"NOT_DOCUMENTED_EVENT",
       ulak::core::SeverityLevel::kError,
       ulak::core::SafetyAction::kRequestConfirmation,
       ulak::core::SafetyAction::kPanicRtl,
       true,
       5},
  };
  if (!VerifyProfileMappings(&profile_manager, &classifier, "default", 5, default_rules)) {
    return false;
  }

  const std::vector<ExpectedRule> safe_rules = {
      {"TELEMETRY_LOSS",
       ulak::core::SeverityLevel::kCritical,
       ulak::core::SafetyAction::kPanicRtl,
       std::nullopt,
       false,
       3},
      {"CC_LINK_LOSS",
       ulak::core::SeverityLevel::kCritical,
       ulak::core::SafetyAction::kPanicRtl,
       std::nullopt,
       false,
       3},
      {"STREAM_LOSS",
       ulak::core::SeverityLevel::kError,
       ulak::core::SafetyAction::kRequestConfirmation,
       ulak::core::SafetyAction::kPanicRtl,
       false,
       3},
      {"VISION_LOST",
       ulak::core::SeverityLevel::kError,
       ulak::core::SafetyAction::kRequestConfirmation,
       ulak::core::SafetyAction::kPanicRtl,
       false,
       3},
      {"LOW_BATTERY",
       ulak::core::SeverityLevel::kCritical,
       ulak::core::SafetyAction::kPanicRtl,
       std::nullopt,
       false,
       3},
      {"INVALID_OPERATOR_ACTION",
       ulak::core::SeverityLevel::kError,
       ulak::core::SafetyAction::kRejectCommand,
       std::nullopt,
       false,
       3},
      {"NOT_DOCUMENTED_EVENT",
       ulak::core::SeverityLevel::kCritical,
       ulak::core::SafetyAction::kPanicRtl,
       std::nullopt,
       true,
       3},
  };
  if (!VerifyProfileMappings(&profile_manager, &classifier, "safe", 3, safe_rules)) {
    return false;
  }

  const std::vector<ExpectedRule> aggressive_rules = {
      {"TELEMETRY_LOSS",
       ulak::core::SeverityLevel::kError,
       ulak::core::SafetyAction::kRequestConfirmation,
       ulak::core::SafetyAction::kHoldPosition,
       false,
       8},
      {"CC_LINK_LOSS",
       ulak::core::SeverityLevel::kWarn,
       ulak::core::SafetyAction::kNotifyOnly,
       std::nullopt,
       false,
       8},
      {"STREAM_LOSS",
       ulak::core::SeverityLevel::kWarn,
       ulak::core::SafetyAction::kNotifyOnly,
       std::nullopt,
       false,
       8},
      {"VISION_LOST",
       ulak::core::SeverityLevel::kWarn,
       ulak::core::SafetyAction::kNotifyOnly,
       std::nullopt,
       false,
       8},
      {"LOW_BATTERY",
       ulak::core::SeverityLevel::kError,
       ulak::core::SafetyAction::kRequestConfirmation,
       ulak::core::SafetyAction::kPanicRtl,
       false,
       8},
      {"INVALID_OPERATOR_ACTION",
       ulak::core::SeverityLevel::kError,
       ulak::core::SafetyAction::kRejectCommand,
       std::nullopt,
       false,
       8},
      {"NOT_DOCUMENTED_EVENT",
       ulak::core::SeverityLevel::kWarn,
       ulak::core::SafetyAction::kNotifyOnly,
       std::nullopt,
       true,
       8},
  };
  if (!VerifyProfileMappings(&profile_manager, &classifier, "aggressive", 8, aggressive_rules)) {
    return false;
  }

  return Expect(!classifier.audit_log().empty(),
                "Expected exception classifier to produce audit records");
}

bool TestProtectedDeleteWorkflow() {
  ulak::core::ProfileManager profile_manager;
  const auto profiles_dir = ResolveRepoRelative("config/profiles");
  const auto load = profile_manager.LoadProfiles(profiles_dir, "default");
  if (!Expect(load.ok, "Expected profile loading to succeed for delete workflow")) {
    return false;
  }

  std::string delete_error;
  if (!Expect(!profile_manager.DeleteProfileInUiWorkflow("default", &delete_error),
              "Expected default profile delete to be rejected")) {
    return false;
  }
  if (!Expect(!delete_error.empty(), "Expected delete rejection reason")) {
    return false;
  }
  if (!Expect(profile_manager.FindProfile("default") != nullptr,
              "Expected default profile to remain loaded")) {
    return false;
  }

  if (!Expect(profile_manager.DeleteProfileInUiWorkflow("safe", &delete_error),
              "Expected non-protected profile delete to succeed")) {
    return false;
  }
  return Expect(profile_manager.FindProfile("safe") == nullptr,
                "Expected safe profile to be removed after delete");
}

}  // namespace

int main() {
  const bool ok = TestProfileManagerAndClassifierMappings() &&
                  TestProtectedDeleteWorkflow();
  if (!ok) {
    return 1;
  }

  std::cout << "[test] Profile manager and mapping tests passed.\n";
  return 0;
}
