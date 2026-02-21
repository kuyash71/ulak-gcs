#pragma once

#include <optional>
#include <string>
#include <vector>

#include "ProfileManager.h"

namespace ulak::core {

enum class ConfirmationState {
  kPending = 0,
  kConfirmed,
  kCanceled,
  kTimedOut
};

const char* ToString(ConfirmationState state);

struct ClassificationResult {
  bool ok{false};
  std::string message;

  std::string profile_id;
  std::string event_code;
  bool used_default_mapping{false};

  SeverityLevel severity{SeverityLevel::kWarn};
  SafetyAction action{SafetyAction::kNotifyOnly};
  std::optional<SafetyAction> timeout_action;
  int confirm_window_sec{0};
};

struct ErrorCountdown {
  std::string profile_id;
  std::string event_code;

  int seconds_remaining{0};
  SafetyAction confirm_action{SafetyAction::kRequestConfirmation};
  SafetyAction timeout_action{SafetyAction::kPanicRtl};
  ConfirmationState state{ConfirmationState::kPending};
};

struct CountdownStep {
  ConfirmationState state{ConfirmationState::kPending};
  int seconds_remaining{0};
  bool action_dispatched{false};
  SafetyAction dispatched_action{SafetyAction::kNotifyOnly};
};

struct SafetyAuditRecord {
  std::string source;
  std::string profile_id;
  std::string event_code;
  SeverityLevel severity{SeverityLevel::kWarn};
  SafetyAction action{SafetyAction::kNotifyOnly};
  std::string correlation_id;
  std::string status;
};

class ExceptionClassifier {
 public:
  explicit ExceptionClassifier(const ProfileManager* profile_manager);

  ClassificationResult ClassifyEvent(const std::string& event_code);
  bool BeginErrorCountdown(const ClassificationResult& classification,
                           ErrorCountdown* countdown,
                           std::string* reason = nullptr);
  CountdownStep Tick(ErrorCountdown* countdown);
  CountdownStep Confirm(ErrorCountdown* countdown);
  CountdownStep Cancel(ErrorCountdown* countdown);

  const std::vector<SafetyAuditRecord>& audit_log() const {
    return audit_log_;
  }

 private:
  void AddAudit(const SafetyAuditRecord& record);
  CountdownStep MakeNoopStep() const;

  const ProfileManager* profile_manager_{nullptr};
  std::vector<SafetyAuditRecord> audit_log_;
};

}  // namespace ulak::core
