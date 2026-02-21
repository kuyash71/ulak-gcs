#include "PanicManager.h"

#include <utility>

namespace ulak::core {

PanicCommandRequest PanicManager::BuildPanicCommand(const std::string& profile_id,
                                                    const std::string& correlation_id,
                                                    const std::string& timestamp) {
  PanicCommandRequest request;
  request.timestamp = timestamp;
  request.correlation_id = correlation_id.empty() ? NextCorrelationId() : correlation_id;

  PanicAuditRecord audit;
  audit.profile_id = profile_id;
  audit.correlation_id = request.correlation_id;
  audit.command = request.command;
  audit.status = "SENT";
  audit.timestamp = timestamp;
  audit_log_.push_back(std::move(audit));

  return request;
}

void PanicManager::RecordLifecycle(const std::string& profile_id,
                                   const std::string& correlation_id,
                                   const std::string& status,
                                   const std::string& timestamp) {
  PanicAuditRecord audit;
  audit.profile_id = profile_id;
  audit.correlation_id = correlation_id;
  audit.command = "PANIC_RTL";
  audit.status = status;
  audit.timestamp = timestamp;
  audit_log_.push_back(std::move(audit));
}

std::string PanicManager::NextCorrelationId() {
  const std::string generated = "panic-" + std::to_string(next_sequence_);
  ++next_sequence_;
  return generated;
}

}  // namespace ulak::core
