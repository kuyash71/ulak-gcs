#pragma once

#include <string>

#include "JsonUtils.h"

namespace ulak::models {

constexpr const char* kCommandRequestCategory = "station/commands/request";

enum class CommandParseError {
  kNone = 0,
  kInvalidJson,
  kMissingField,
  kInvalidType,
  kInvalidValue,
  kUnsupportedSchema,
  kUnsupportedCategory
};

struct CommandParseResult {
  bool ok{false};
  CommandParseError error{CommandParseError::kNone};
  std::string message;
};

struct CommandRequest {
  std::string schema_version{"1.0.0"};
  std::string timestamp;
  std::string source;
  std::string correlation_id;
  std::string command;
  std::string target;
  ulak::json::Value params;  // object payload (may be empty)
};

CommandParseResult ParseCommandRequest(const std::string& json, CommandRequest* out);
std::string SerializeCommandRequest(const CommandRequest& request);

const char* ToString(CommandParseError error);

}  // namespace ulak::models
