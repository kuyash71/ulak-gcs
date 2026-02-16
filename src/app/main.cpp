#include "BootstrapConfig.h"

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
      continue;
    }

    *error = "Unknown argument: " + std::string(arg);
    return false;
  }

  return true;
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
  const auto validation = ulak::bootstrap::ValidateConfigFile(options.config_path);
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
