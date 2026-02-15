#include "BootstrapConfig.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {

bool Expect(bool condition, const std::string& failure_message) {
  if (!condition) {
    std::cerr << "[test] " << failure_message << '\n';
    return false;
  }
  return true;
}

bool WriteFile(const std::filesystem::path& path, const std::string& content) {
  std::ofstream output(path);
  if (!output.good()) {
    return false;
  }
  output << content;
  return output.good();
}

bool TestValidConfig() {
  const auto result = ulak::bootstrap::ValidateConfigFile("config/settings.json");
  return Expect(result.ok, "Expected config/settings.json to validate") &&
         Expect(!result.schema_version.empty(), "Expected non-empty schema_version") &&
         Expect(!result.active_profile.empty(), "Expected non-empty active_profile");
}

bool TestMissingConfigFile() {
  const auto result = ulak::bootstrap::ValidateConfigFile("config/not_found.json");
  return Expect(!result.ok, "Expected missing config to fail validation") &&
         Expect(result.error == ulak::bootstrap::ValidationError::kMissingFile,
                "Expected kMissingFile for missing config");
}

bool TestInvalidJson(const std::filesystem::path& temp_dir) {
  const auto invalid_path = temp_dir / "invalid.json";
  if (!WriteFile(invalid_path, R"({"schema_version":"1.0.0", "active_profile": })")) {
    return Expect(false, "Failed to write invalid.json");
  }

  const auto result = ulak::bootstrap::ValidateConfigFile(invalid_path);
  return Expect(!result.ok, "Expected malformed JSON to fail validation") &&
         Expect(result.error == ulak::bootstrap::ValidationError::kInvalidJson,
                "Expected kInvalidJson for malformed JSON");
}

bool TestMissingRequiredField(const std::filesystem::path& temp_dir) {
  const auto invalid_path = temp_dir / "missing_active_profile.json";
  const std::string content = R"({
    "schema_version": "1.0.0",
    "telemetry": {},
    "companion": {},
    "stream": {}
  })";
  if (!WriteFile(invalid_path, content)) {
    return Expect(false, "Failed to write missing_active_profile.json");
  }

  const auto result = ulak::bootstrap::ValidateConfigFile(invalid_path);
  return Expect(!result.ok, "Expected missing active_profile to fail validation") &&
         Expect(result.error == ulak::bootstrap::ValidationError::kMissingRequiredField,
                "Expected kMissingRequiredField for missing active_profile");
}

bool TestInvalidFieldType(const std::filesystem::path& temp_dir) {
  const auto invalid_path = temp_dir / "invalid_type.json";
  const std::string content = R"({
    "schema_version": "1.0.0",
    "active_profile": 42,
    "telemetry": {},
    "companion": {},
    "stream": {}
  })";
  if (!WriteFile(invalid_path, content)) {
    return Expect(false, "Failed to write invalid_type.json");
  }

  const auto result = ulak::bootstrap::ValidateConfigFile(invalid_path);
  return Expect(!result.ok, "Expected invalid field type to fail validation") &&
         Expect(result.error == ulak::bootstrap::ValidationError::kInvalidFieldType,
                "Expected kInvalidFieldType for active_profile number");
}

}  // namespace

int main() {
  std::error_code ec;
  const auto temp_dir = std::filesystem::temp_directory_path(ec) / "ulak_gcs_bootstrap_tests";
  if (ec) {
    std::cerr << "[test] Failed to access temporary directory: " << ec.message() << '\n';
    return 1;
  }
  std::filesystem::create_directories(temp_dir, ec);
  if (ec) {
    std::cerr << "[test] Failed to create temporary directory: " << ec.message() << '\n';
    return 1;
  }

  const bool ok = TestValidConfig() &&
                  TestMissingConfigFile() &&
                  TestInvalidJson(temp_dir) &&
                  TestMissingRequiredField(temp_dir) &&
                  TestInvalidFieldType(temp_dir);

  std::filesystem::remove_all(temp_dir, ec);
  if (ec) {
    std::cerr << "[test] Warning: failed to clean temporary files: " << ec.message() << '\n';
  }

  if (!ok) {
    return 1;
  }

  std::cout << "[test] Bootstrap config validation tests passed.\n";
  return 0;
}
