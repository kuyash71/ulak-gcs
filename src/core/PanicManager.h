#pragma once

#include <string>
#include <vector>

namespace ulak::core {

struct PanicCommandRequest {
  std::string schema_version{"1.0.0"};
  std::string category{"station/commands/request"};
  std::string timestamp;
  std::string source{"station"};
  std::string correlation_id;

  std::string command{"PANIC_RTL"};
  std::string target{"flight_controller"};
};

struct PanicAuditRecord {
  std::string profile_id;
  std::string correlation_id;
  std::string command;
  std::string status;
  std::string timestamp;
};

class PanicManager {
 public:
  PanicCommandRequest BuildPanicCommand(const std::string& profile_id,
                                        const std::string& correlation_id,
                                        const std::string& timestamp);

  void RecordLifecycle(const std::string& profile_id,
                       const std::string& correlation_id,
                       const std::string& status,
                       const std::string& timestamp);

  const std::vector<PanicAuditRecord>& audit_log() const {
    return audit_log_;
  }

 private:
  std::string NextCorrelationId();

  std::vector<PanicAuditRecord> audit_log_;
  unsigned long next_sequence_{1};
};

}  // namespace ulak::core
