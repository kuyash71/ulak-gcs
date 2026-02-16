#include "BootstrapConfig.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>

namespace {

// Exit codes aligned with common UNIX conventions (sysexits-style).
constexpr int kExitOk = 0;
constexpr int kExitInvalidUsage = 64;
constexpr int kExitConfigError = 78;

struct CliOptions {
  std::string config_path = "config/settings.json";
  bool config_explicit = false;
  bool validate_only = false;
  bool show_help = false;
};

// Simple CLI help output.
void PrintHelp(const char* executable_name) {
  std::cout << "ULAK GCS bootstrap executable\n"
            << "Usage: " << executable_name
            << " [--config <path>] [--validate-only] [--help]\n\n"
            << "Options:\n"
            << "  --config <path>   Use a custom settings file.\n"
            << "  --validate-only   Validate config and exit.\n"
            << "  --help            Show this help text.\n";
}

// Parses CLI flags; unknown arguments are treated as errors.
bool ParseArgs(int argc, char** argv, CliOptions* options, std::string* error) {
  for (int i = 1; i < argc; ++i) {
    const std::string_view arg(argv[i]);

    if (arg == "--help" || arg == "-h") {
      options->show_help = true;
      continue;
    }

    if (arg == "--validate-only") {
      options->validate_only = true;
      continue;
    }

    if (arg == "--config") {
      if (i + 1 >= argc) {
        *error = "Missing value for --config";
        return false;
      }
      options->config_path = argv[++i];
      options->config_explicit = true;
      continue;
    }

    *error = "Unknown argument: " + std::string(arg);
    return false;
  }

  return true;
}

// Attempts to resolve a relative config path by walking parent directories.
std::filesystem::path ResolveConfigPath(const std::filesystem::path& requested_path) {
  if (!requested_path.is_relative()) {
    return requested_path;
  }

  std::error_code ec;
  std::filesystem::path current = std::filesystem::current_path(ec);
  if (ec) {
    return requested_path;
  }

  for (int depth = 0; depth < 6; ++depth) {
    const auto candidate = current / requested_path;
    if (std::filesystem::exists(candidate, ec) && !ec) {
      return candidate;
    }
    if (current == current.root_path()) {
      break;
    }
    current = current.parent_path();
  }

  return requested_path;
}

}  // namespace

int main(int argc, char** argv) {
  CliOptions options;
  std::string argument_error;
  if (!ParseArgs(argc, argv, &options, &argument_error)) {
    std::cerr << "[sauro_station] " << argument_error << '\n';
    PrintHelp(argv[0]);
    return kExitInvalidUsage;
  }

  if (options.show_help) {
    PrintHelp(argv[0]);
    return kExitOk;
  }

  // Validate config before proceeding with any app startup.
  std::filesystem::path config_path = options.config_path;
  if (!options.config_explicit) {
    // Resolve default config path even when the app is started from build/.
    config_path = ResolveConfigPath(config_path);
  }

  const auto validation = ulak::bootstrap::ValidateConfigFile(config_path);
  if (!validation.ok) {
    std::cerr << "[sauro_station] Config validation failed ("
              << ulak::bootstrap::ToString(validation.error)
              << "): " << validation.message << '\n';
    return kExitConfigError;
  }

  std::cout << "[sauro_station] Bootstrap OK. "
            << "schema_version=" << validation.schema_version
            << ", active_profile=" << validation.active_profile << '\n';

  // Validate-only mode exits without starting the UI/event loop.
  if (options.validate_only) {
    std::cout << "[sauro_station] validate-only mode complete. Clean shutdown.\n";
    return kExitOk;
  }

  std::cout << "[sauro_station] Startup smoke path complete. Clean shutdown.\n";
  return kExitOk;
}
