#include "ConfigLoader.h"
#include "ProfileLoader.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

std::filesystem::path ResolveRepoRelative(const std::filesystem::path& relative_path) {
  if (!relative_path.is_relative()) {
    return relative_path;
  }
  std::error_code ec;
  std::filesystem::path current = std::filesystem::current_path(ec);
  if (ec) {
    return relative_path;
  }
  for (int depth = 0; depth < 6; ++depth) {
    const auto candidate = current / relative_path;
    if (std::filesystem::exists(candidate, ec) && !ec) {
      return candidate;
    }
    if (current == current.root_path()) {
      break;
    }
    current = current.parent_path();
  }
  return relative_path;
}

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

std::string MinimalValidConfigJson(const std::string& schema_version,
                                   const std::string& stream_mode) {
  std::ostringstream out;
  out << "{\n"
      << "  \"schema_version\": \"" << schema_version << "\",\n"
      << "  \"instance_name\": \"test-instance\",\n"
      << "  \"active_profile\": \"default\",\n"
      << "  \"telemetry\": {\n"
      << "    \"vehicle_endpoint\": { \"transport\": \"udp\", \"host\": \"127.0.0.1\", \"port\": 14550 },\n"
      << "    \"simulator_endpoint\": { \"transport\": \"udp\", \"host\": \"127.0.0.1\", \"port\": 14560 },\n"
      << "    \"health_interval_ms\": 500\n"
      << "  },\n"
      << "  \"companion\": {\n"
      << "    \"endpoint\": { \"transport\": \"tcp\", \"host\": \"127.0.0.1\", \"port\": 5760 },\n"
      << "    \"command_endpoint\": { \"transport\": \"tcp\", \"host\": \"127.0.0.1\", \"port\": 5770 }\n"
      << "  },\n"
      << "  \"stream\": { \"mode\": \"" << stream_mode << "\" }\n"
      << "}\n";
  return out.str();
}

bool TestValidConfig() {
  ulak::core::AppConfig config;
  const auto settings_path = ResolveRepoRelative("config/settings.json");
  const auto result = ulak::core::LoadConfigFile(settings_path, &config);
  return Expect(result.ok, "Expected config/settings.json to load") &&
         Expect(config.schema_version == "1.0.0", "Expected schema_version=1.0.0") &&
         Expect(!config.active_profile.empty(), "Expected active_profile to be set");
}

bool TestMissingConfigFile() {
  ulak::core::AppConfig config;
  const auto missing_path = ResolveRepoRelative("config/not_found.json");
  const auto result = ulak::core::LoadConfigFile(missing_path, &config);
  return Expect(!result.ok, "Expected missing config to fail") &&
         Expect(result.error == ulak::core::ConfigError::kMissingFile,
                "Expected kMissingFile for missing config");
}

bool TestInvalidJson(const std::filesystem::path& temp_dir) {
  const auto invalid_path = temp_dir / "invalid.json";
  if (!WriteFile(invalid_path, R"({"schema_version":"1.0.0", "active_profile": })")) {
    return Expect(false, "Failed to write invalid.json");
  }

  ulak::core::AppConfig config;
  const auto result = ulak::core::LoadConfigFile(invalid_path, &config);
  return Expect(!result.ok, "Expected malformed JSON to fail") &&
         Expect(result.error == ulak::core::ConfigError::kInvalidJson,
                "Expected kInvalidJson for malformed JSON");
}

bool TestUnsupportedSchema(const std::filesystem::path& temp_dir) {
  const auto path = temp_dir / "schema.json";
  if (!WriteFile(path, MinimalValidConfigJson("9.9.9", "OFF"))) {
    return Expect(false, "Failed to write schema.json");
  }

  ulak::core::AppConfig config;
  const auto result = ulak::core::LoadConfigFile(path, &config);
  return Expect(!result.ok, "Expected unsupported schema to fail") &&
         Expect(result.error == ulak::core::ConfigError::kUnsupportedSchema,
                "Expected kUnsupportedSchema for schema mismatch");
}

bool TestInvalidValue(const std::filesystem::path& temp_dir) {
  const auto path = temp_dir / "invalid_mode.json";
  if (!WriteFile(path, MinimalValidConfigJson("1.0.0", "NOT_A_MODE"))) {
    return Expect(false, "Failed to write invalid_mode.json");
  }

  ulak::core::AppConfig config;
  const auto result = ulak::core::LoadConfigFile(path, &config);
  return Expect(!result.ok, "Expected invalid stream mode to fail") &&
         Expect(result.error == ulak::core::ConfigError::kInvalidValue,
                "Expected kInvalidValue for invalid stream.mode");
}

bool TestMissingField(const std::filesystem::path& temp_dir) {
  const auto path = temp_dir / "missing_field.json";
  const std::string content = R"({
    "schema_version": "1.0.0",
    "instance_name": "test-instance",
    "active_profile": "default",
    "companion": {
      "endpoint": { "transport": "tcp", "host": "127.0.0.1", "port": 5760 },
      "command_endpoint": { "transport": "tcp", "host": "127.0.0.1", "port": 5770 }
    },
    "stream": { "mode": "OFF" }
  })";
  if (!WriteFile(path, content)) {
    return Expect(false, "Failed to write missing_field.json");
  }

  ulak::core::AppConfig config;
  const auto result = ulak::core::LoadConfigFile(path, &config);
  return Expect(!result.ok, "Expected missing telemetry to fail") &&
         Expect(result.error == ulak::core::ConfigError::kMissingField,
                "Expected kMissingField for missing telemetry");
}

bool TestProfileSchemaFallback(const std::filesystem::path& temp_dir) {
  const auto path = temp_dir / "profile_schema.json";
  const std::string content = R"({
    "schema_version": "9.9.9",
    "profile_id": "broken",
    "display_name": "Broken",
    "protected": false
  })";
  if (!WriteFile(path, content)) {
    return Expect(false, "Failed to write profile_schema.json");
  }

  ulak::core::ProfileConfig profile;
  const auto fallback = ResolveRepoRelative("config/profiles/default.json");
  const auto result = ulak::core::LoadProfileFile(path, fallback, &profile);
  return Expect(result.ok, "Expected fallback profile load to succeed") &&
         Expect(result.used_fallback, "Expected fallback to be used") &&
         Expect(profile.profile_id == "default", "Expected fallback profile_id=default");
}

bool TestProfileMissingFile() {
  ulak::core::ProfileConfig profile;
  const auto missing = ResolveRepoRelative("config/profiles/not_found.json");
  const auto fallback = ResolveRepoRelative("config/profiles/default.json");
  const auto result = ulak::core::LoadProfileFile(missing, fallback, &profile);
  return Expect(!result.ok, "Expected missing profile to fail") &&
         Expect(result.error == ulak::core::ConfigError::kMissingFile,
                "Expected kMissingFile for missing profile");
}

}  // namespace

int main() {
  std::error_code ec;
  const auto temp_dir = std::filesystem::temp_directory_path(ec) / "ulak_gcs_config_tests";
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
                  TestUnsupportedSchema(temp_dir) &&
                  TestInvalidValue(temp_dir) &&
                  TestMissingField(temp_dir) &&
                  TestProfileSchemaFallback(temp_dir) &&
                  TestProfileMissingFile();

  std::filesystem::remove_all(temp_dir, ec);
  if (ec) {
    std::cerr << "[test] Warning: failed to clean temporary files: " << ec.message() << '\n';
  }

  if (!ok) {
    return 1;
  }

  std::cout << "[test] Config loader tests passed.\n";
  return 0;
}
