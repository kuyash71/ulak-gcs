#include "ExceptionClassifier.h"

#include <utility>

namespace ulak::core {
namespace {

ClassificationResult MakeError(std::string message) {
  ClassificationResult result;
  result.ok = false;
  result.message = std::move(message);
  return result;
}

}  // namespace

const char* ToString(ConfirmationState state) {
  switch (state) {
    case ConfirmationState::kPending:
      return "PENDING";
    case ConfirmationState::kConfirmed:
      return "CONFIRMED";
    case ConfirmationState::kCanceled:
      return "CANCELED";
    case ConfirmationState::kTimedOut:
      return "TIMED_OUT";
  }
  return "PENDING";
}

ExceptionClassifier::ExceptionClassifier(const ProfileManager* profile_manager)
    : profile_manager_(profile_manager) {}

ClassificationResult ExceptionClassifier::ClassifyEvent(const std::string& event_code) {
  if (!profile_manager_) {
    return MakeError("Profile manager is null");
  }

  const ProfileData* active_profile = profile_manager_->GetActiveProfile();
  if (!active_profile) {
    return MakeError("No active profile loaded");
  }

  ClassificationResult result;
  result.ok = true;
  result.message = "Event classified";
  result.profile_id = active_profile->profile_id;
  result.event_code = event_code;
  result.confirm_window_sec = active_profile->error_confirm_window_sec;
  result.used_default_mapping = true;
  result.severity = active_profile->unknown_event.severity;
  result.action = active_profile->unknown_event.action;
  result.timeout_action = active_profile->unknown_event.timeout_action;

  for (const auto& mapping : active_profile->mappings) {
    if (mapping.event_code != event_code) {
      continue;
    }
    result.used_default_mapping = false;
    result.severity = mapping.severity;
    result.action = mapping.action;
    result.timeout_action = mapping.timeout_action;
    break;
  }

  SafetyAuditRecord record;
  record.source = "exception_classifier";
  record.profile_id = result.profile_id;
  record.event_code = result.event_code;
  record.severity = result.severity;
  record.action = result.action;
  record.status = "CLASSIFIED";
  AddAudit(record);

  return result;
}

bool ExceptionClassifier::BeginErrorCountdown(const ClassificationResult& classification,
                                              ErrorCountdown* countdown,
                                              std::string* reason) {
  if (!countdown) {
    if (reason) {
      *reason = "Countdown output is null";
    }
    return false;
  }
  if (!classification.ok) {
    if (reason) {
      *reason = "Cannot start countdown for failed classification";
    }
    return false;
  }
  if (classification.severity != SeverityLevel::kError) {
    if (reason) {
      *reason = "Countdown is only valid for ERROR severity";
    }
    return false;
  }
  if (classification.action != SafetyAction::kRequestConfirmation) {
    if (reason) {
      *reason = "Countdown requires REQUEST_CONFIRMATION action";
    }
    return false;
  }

  ErrorCountdown session;
  session.profile_id = classification.profile_id;
  session.event_code = classification.event_code;
  session.seconds_remaining = classification.confirm_window_sec > 0 ? classification.confirm_window_sec : 5;
  session.confirm_action = classification.action;
  session.timeout_action = classification.timeout_action.value_or(SafetyAction::kPanicRtl);
  session.state = ConfirmationState::kPending;
  *countdown = std::move(session);

  SafetyAuditRecord record;
  record.source = "exception_classifier";
  record.profile_id = countdown->profile_id;
  record.event_code = countdown->event_code;
  record.severity = SeverityLevel::kError;
  record.action = countdown->confirm_action;
  record.status = "COUNTDOWN_STARTED";
  AddAudit(record);

  return true;
}

CountdownStep ExceptionClassifier::Tick(ErrorCountdown* countdown) {
  if (!countdown) {
    return MakeNoopStep();
  }

  CountdownStep step;
  step.state = countdown->state;
  step.seconds_remaining = countdown->seconds_remaining;
  step.action_dispatched = false;
  step.dispatched_action = countdown->timeout_action;

  if (countdown->state != ConfirmationState::kPending) {
    return step;
  }

  if (countdown->seconds_remaining > 0) {
    --countdown->seconds_remaining;
  }

  if (countdown->seconds_remaining > 0) {
    step.state = ConfirmationState::kPending;
    step.seconds_remaining = countdown->seconds_remaining;
    return step;
  }

  countdown->state = ConfirmationState::kTimedOut;
  step.state = countdown->state;
  step.seconds_remaining = 0;
  step.action_dispatched = true;
  step.dispatched_action = countdown->timeout_action;

  SafetyAuditRecord record;
  record.source = "exception_classifier";
  record.profile_id = countdown->profile_id;
  record.event_code = countdown->event_code;
  record.severity = SeverityLevel::kError;
  record.action = countdown->timeout_action;
  record.status = "TIMEOUT_DISPATCHED";
  AddAudit(record);

  return step;
}

CountdownStep ExceptionClassifier::Confirm(ErrorCountdown* countdown) {
  if (!countdown) {
    return MakeNoopStep();
  }

  CountdownStep step;
  step.state = countdown->state;
  step.seconds_remaining = countdown->seconds_remaining;
  step.action_dispatched = false;
  step.dispatched_action = countdown->confirm_action;

  if (countdown->state != ConfirmationState::kPending) {
    return step;
  }

  countdown->state = ConfirmationState::kConfirmed;
  step.state = countdown->state;
  step.action_dispatched = true;
  step.dispatched_action = countdown->confirm_action;

  SafetyAuditRecord record;
  record.source = "exception_classifier";
  record.profile_id = countdown->profile_id;
  record.event_code = countdown->event_code;
  record.severity = SeverityLevel::kError;
  record.action = countdown->confirm_action;
  record.status = "CONFIRM_DISPATCHED";
  AddAudit(record);

  return step;
}

CountdownStep ExceptionClassifier::Cancel(ErrorCountdown* countdown) {
  if (!countdown) {
    return MakeNoopStep();
  }

  CountdownStep step;
  step.state = countdown->state;
  step.seconds_remaining = countdown->seconds_remaining;
  step.action_dispatched = false;
  step.dispatched_action = countdown->confirm_action;

  if (countdown->state != ConfirmationState::kPending) {
    return step;
  }

  countdown->state = ConfirmationState::kCanceled;
  step.state = countdown->state;
  step.action_dispatched = false;

  SafetyAuditRecord record;
  record.source = "exception_classifier";
  record.profile_id = countdown->profile_id;
  record.event_code = countdown->event_code;
  record.severity = SeverityLevel::kError;
  record.action = countdown->confirm_action;
  record.status = "CANCELED";
  AddAudit(record);

  return step;
}

void ExceptionClassifier::AddAudit(const SafetyAuditRecord& record) {
  audit_log_.push_back(record);
}

CountdownStep ExceptionClassifier::MakeNoopStep() const {
  CountdownStep step;
  step.state = ConfirmationState::kPending;
  step.seconds_remaining = 0;
  step.action_dispatched = false;
  step.dispatched_action = SafetyAction::kNotifyOnly;
  return step;
}

}  // namespace ulak::core
