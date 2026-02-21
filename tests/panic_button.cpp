#include "PanicManager.h"
#include "ProfileManager.h"

#include <filesystem>
#include <iostream>
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

bool TestPanicRtlAcrossProfiles() {
  ulak::core::ProfileManager profile_manager;
  const auto load = profile_manager.LoadProfiles(ResolveRepoRelative("config/profiles"), "default");
  if (!Expect(load.ok, "Expected profile loading to succeed for panic test")) {
    return false;
  }

  ulak::core::PanicManager panic_manager;
  const std::vector<std::string> profiles = {"default", "safe", "aggressive"};

  int correlation_counter = 1;
  for (const auto& profile_id : profiles) {
    std::string reason;
    if (!Expect(profile_manager.SwitchActiveProfile(profile_id, &reason),
                "Expected profile switch for panic test: " + reason)) {
      return false;
    }

    const std::string correlation_id = "panic-test-" + std::to_string(correlation_counter);
    ++correlation_counter;
    const auto request =
        panic_manager.BuildPanicCommand(profile_id, correlation_id, "2026-02-21T00:00:00Z");
    if (!Expect(request.command == "PANIC_RTL", "Panic command must always be PANIC_RTL")) {
      return false;
    }
    if (!Expect(request.target == "flight_controller",
                "Panic target must remain flight_controller")) {
      return false;
    }
    if (!Expect(request.category == "station/commands/request",
                "Panic category must stay station/commands/request")) {
      return false;
    }
    if (!Expect(request.correlation_id == correlation_id,
                "Panic must preserve provided correlation id")) {
      return false;
    }

    panic_manager.RecordLifecycle(profile_id, request.correlation_id, "ACK", "2026-02-21T00:00:01Z");
    panic_manager.RecordLifecycle(profile_id,
                                  request.correlation_id,
                                  "EXEC_TIMEOUT",
                                  "2026-02-21T00:00:10Z");
  }

  const auto& audit = panic_manager.audit_log();
  if (!Expect(audit.size() == profiles.size() * 3,
              "Expected SENT + lifecycle audit for each profile panic")) {
    return false;
  }
  for (const auto& item : audit) {
    if (!Expect(item.command == "PANIC_RTL", "Audit command must stay PANIC_RTL")) {
      return false;
    }
  }

  return true;
}

bool TestGeneratedCorrelationIdWhenMissing() {
  ulak::core::PanicManager panic_manager;
  const auto first = panic_manager.BuildPanicCommand("default", "", "2026-02-21T00:00:00Z");
  const auto second = panic_manager.BuildPanicCommand("default", "", "2026-02-21T00:00:01Z");
  if (!Expect(!first.correlation_id.empty(), "Expected generated correlation id for first panic")) {
    return false;
  }
  if (!Expect(!second.correlation_id.empty(), "Expected generated correlation id for second panic")) {
    return false;
  }
  return Expect(first.correlation_id != second.correlation_id,
                "Generated correlation ids must be unique");
}

}  // namespace

int main() {
  const bool ok = TestPanicRtlAcrossProfiles() &&
                  TestGeneratedCorrelationIdWhenMissing();
  if (!ok) {
    return 1;
  }

  std::cout << "[test] Panic manager tests passed.\n";
  return 0;
}
