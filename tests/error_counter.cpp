#include "ExceptionClassifier.h"
#include "ProfileManager.h"

#include <filesystem>
#include <iostream>
#include <string>

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

bool TestTimeoutCountdownPath() {
  ulak::core::ProfileManager profile_manager;
  const auto load = profile_manager.LoadProfiles(ResolveRepoRelative("config/profiles"), "default");
  if (!Expect(load.ok, "Expected profile loading to succeed for countdown timeout test")) {
    return false;
  }

  ulak::core::ExceptionClassifier classifier(&profile_manager);
  const auto classification = classifier.ClassifyEvent("TELEMETRY_LOSS");
  if (!Expect(classification.ok, "Expected TELEMETRY_LOSS classification success")) {
    return false;
  }
  if (!Expect(classification.confirm_window_sec == 5, "Expected default countdown window 5")) {
    return false;
  }

  ulak::core::ErrorCountdown countdown;
  std::string reason;
  if (!Expect(classifier.BeginErrorCountdown(classification, &countdown, &reason),
              "Expected countdown start success: " + reason)) {
    return false;
  }
  if (!Expect(countdown.seconds_remaining == 5, "Expected countdown to start at 5")) {
    return false;
  }

  for (int second = 4; second >= 1; --second) {
    const auto step = classifier.Tick(&countdown);
    if (!Expect(step.state == ulak::core::ConfirmationState::kPending,
                "Expected pending state before timeout")) {
      return false;
    }
    if (!Expect(step.seconds_remaining == second,
                "Unexpected remaining seconds during countdown")) {
      return false;
    }
    if (!Expect(!step.action_dispatched, "No action should be dispatched while pending")) {
      return false;
    }
  }

  const auto timeout_step = classifier.Tick(&countdown);
  if (!Expect(timeout_step.state == ulak::core::ConfirmationState::kTimedOut,
              "Expected timed out state at zero")) {
    return false;
  }
  if (!Expect(timeout_step.seconds_remaining == 0, "Expected zero remaining seconds")) {
    return false;
  }
  if (!Expect(timeout_step.action_dispatched, "Expected fallback action on timeout")) {
    return false;
  }
  return Expect(timeout_step.dispatched_action == ulak::core::SafetyAction::kPanicRtl,
                "Expected PANIC_RTL fallback for default profile timeout");
}

bool TestConfirmAndCancelBranches() {
  ulak::core::ProfileManager profile_manager;
  const auto load = profile_manager.LoadProfiles(ResolveRepoRelative("config/profiles"), "default");
  if (!Expect(load.ok, "Expected profile loading to succeed for confirm/cancel test")) {
    return false;
  }

  ulak::core::ExceptionClassifier classifier(&profile_manager);
  const auto classification = classifier.ClassifyEvent("CC_LINK_LOSS");
  if (!Expect(classification.ok, "Expected CC_LINK_LOSS classification success")) {
    return false;
  }

  ulak::core::ErrorCountdown confirm_countdown;
  std::string reason;
  if (!Expect(classifier.BeginErrorCountdown(classification, &confirm_countdown, &reason),
              "Expected confirm countdown start: " + reason)) {
    return false;
  }
  const auto confirmed = classifier.Confirm(&confirm_countdown);
  if (!Expect(confirmed.state == ulak::core::ConfirmationState::kConfirmed,
              "Expected confirm branch state")) {
    return false;
  }
  if (!Expect(confirmed.action_dispatched, "Expected action dispatch on confirm")) {
    return false;
  }
  if (!Expect(confirmed.dispatched_action == ulak::core::SafetyAction::kRequestConfirmation,
              "Expected mapped action on confirm")) {
    return false;
  }

  ulak::core::ErrorCountdown cancel_countdown;
  if (!Expect(classifier.BeginErrorCountdown(classification, &cancel_countdown, &reason),
              "Expected cancel countdown start: " + reason)) {
    return false;
  }
  const auto canceled = classifier.Cancel(&cancel_countdown);
  if (!Expect(canceled.state == ulak::core::ConfirmationState::kCanceled,
              "Expected cancel branch state")) {
    return false;
  }
  return Expect(!canceled.action_dispatched, "Expected no action dispatch on cancel");
}

bool TestConfigurableWindowByProfile() {
  ulak::core::ProfileManager profile_manager;
  const auto load = profile_manager.LoadProfiles(ResolveRepoRelative("config/profiles"), "safe");
  if (!Expect(load.ok, "Expected profile loading to succeed for profile timer test")) {
    return false;
  }

  std::string reason;
  if (!Expect(profile_manager.SwitchActiveProfile("safe", &reason),
              "Expected switch to safe profile: " + reason)) {
    return false;
  }
  ulak::core::ExceptionClassifier classifier(&profile_manager);
  const auto safe_event = classifier.ClassifyEvent("VISION_LOST");
  if (!Expect(safe_event.ok, "Expected safe profile VISION_LOST classification")) {
    return false;
  }
  if (!Expect(safe_event.confirm_window_sec == 3, "Expected safe countdown window 3")) {
    return false;
  }

  ulak::core::ErrorCountdown safe_countdown;
  if (!Expect(classifier.BeginErrorCountdown(safe_event, &safe_countdown, &reason),
              "Expected safe countdown start: " + reason)) {
    return false;
  }
  if (!Expect(safe_countdown.seconds_remaining == 3,
              "Expected safe countdown to start at 3")) {
    return false;
  }

  if (!Expect(profile_manager.SwitchActiveProfile("aggressive", &reason),
              "Expected switch to aggressive profile: " + reason)) {
    return false;
  }
  const auto aggressive_event = classifier.ClassifyEvent("LOW_BATTERY");
  if (!Expect(aggressive_event.ok, "Expected aggressive profile LOW_BATTERY classification")) {
    return false;
  }
  return Expect(aggressive_event.confirm_window_sec == 8,
                "Expected aggressive countdown window 8");
}

}  // namespace

int main() {
  const bool ok = TestTimeoutCountdownPath() &&
                  TestConfirmAndCancelBranches() &&
                  TestConfigurableWindowByProfile();
  if (!ok) {
    return 1;
  }

  std::cout << "[test] Error countdown behavior tests passed.\n";
  return 0;
}
