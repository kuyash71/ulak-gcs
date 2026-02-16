#pragma once

#include <filesystem>
#include <string>

#include "AppConfig.h"
#include "ConfigLoader.h"

namespace ulak::core {

struct ProfileConfig {
  std::string schema_version;
  std::string profile_id;
  std::string display_name;
  bool protected_profile{false};
};

struct ProfileLoadResult {
  bool ok{false};
  bool used_fallback{false};
  ConfigError error{ConfigError::kNone};
  std::string message;
};

// Loads a profile file. If schema mismatches, fallback_path is used.
ProfileLoadResult LoadProfileFile(const std::filesystem::path& path,
                                  const std::filesystem::path& fallback_path,
                                  ProfileConfig* out);

}  // namespace ulak::core
