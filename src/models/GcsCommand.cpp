#include "GcsCommand.h"

#include <utility>

namespace ulak::models {
namespace {

CommandParseResult MakeError(CommandParseError error, std::string message) {
  CommandParseResult result;
  result.ok = false;
  result.error = error;
  result.message = std::move(message);
  return result;
}

bool RequireStringField(const ulak::json::Value& object,
                        const std::string& key,
                        std::string* out,
                        CommandParseResult* error) {
  const ulak::json::Value* value = ulak::json::GetObjectField(object, key);
  if (!value) {
    if (error) {
      *error = MakeError(CommandParseError::kMissingField,
                         "Required field '" + key + "' is missing");
    }
    return false;
  }
  if (value->type != ulak::json::Type::kString) {
    if (error) {
      *error = MakeError(CommandParseError::kInvalidType,
                         "Field '" + key + "' must be a string");
    }
    return false;
  }
  if (value->string_value.empty()) {
    if (error) {
      *error = MakeError(CommandParseError::kInvalidValue,
                         "Field '" + key + "' must not be empty");
    }
    return false;
  }
  *out = value->string_value;
  return true;
}

ulak::json::Value MakeString(const std::string& value) {
  ulak::json::Value v;
  v.type = ulak::json::Type::kString;
  v.string_value = value;
  return v;
}

ulak::json::Value MakeObject() {
  ulak::json::Value v;
  v.type = ulak::json::Type::kObject;
  return v;
}

}  // namespace

CommandParseResult ParseCommandRequest(const std::string& json, CommandRequest* out) {
  if (!out) {
    return MakeError(CommandParseError::kInvalidValue, "Output command pointer is null");
  }

  ulak::json::Value root;
  std::string parse_error;
  if (!ulak::json::Parse(json, &root, &parse_error)) {
    return MakeError(CommandParseError::kInvalidJson, parse_error);
  }
  if (root.type != ulak::json::Type::kObject) {
    return MakeError(CommandParseError::kInvalidType, "Root JSON must be an object");
  }

  CommandRequest request;
  CommandParseResult error;

  if (!RequireStringField(root, "schema_version", &request.schema_version, &error)) {
    return error;
  }
  if (request.schema_version != "1.0.0") {
    return MakeError(CommandParseError::kUnsupportedSchema,
                     "Unsupported schema_version: " + request.schema_version);
  }

  std::string category;
  if (!RequireStringField(root, "category", &category, &error)) {
    return error;
  }
  if (category != kCommandRequestCategory) {
    return MakeError(CommandParseError::kUnsupportedCategory,
                     "Unsupported category: " + category);
  }

  if (!RequireStringField(root, "timestamp", &request.timestamp, &error)) {
    return error;
  }
  if (!RequireStringField(root, "source", &request.source, &error)) {
    return error;
  }
  if (!RequireStringField(root, "correlation_id", &request.correlation_id, &error)) {
    return error;
  }

  const ulak::json::Value* payload = ulak::json::GetObjectField(root, "payload");
  if (!payload) {
    return MakeError(CommandParseError::kMissingField, "Required field 'payload' is missing");
  }
  if (payload->type != ulak::json::Type::kObject) {
    return MakeError(CommandParseError::kInvalidType, "Field 'payload' must be an object");
  }

  if (!RequireStringField(*payload, "command", &request.command, &error)) {
    return error;
  }
  if (!RequireStringField(*payload, "target", &request.target, &error)) {
    return error;
  }

  const ulak::json::Value* params = ulak::json::GetObjectField(*payload, "params");
  if (params) {
    if (params->type != ulak::json::Type::kObject) {
      return MakeError(CommandParseError::kInvalidType, "Field 'params' must be an object");
    }
    request.params = *params;
  } else {
    request.params = MakeObject();
  }

  *out = std::move(request);
  CommandParseResult ok;
  ok.ok = true;
  ok.error = CommandParseError::kNone;
  ok.message = "Command parsed successfully";
  return ok;
}

std::string SerializeCommandRequest(const CommandRequest& request) {
  ulak::json::Value root = MakeObject();
  root.object_value["schema_version"] = MakeString(request.schema_version);
  root.object_value["category"] = MakeString(kCommandRequestCategory);
  root.object_value["timestamp"] = MakeString(request.timestamp);
  root.object_value["source"] = MakeString(request.source);
  root.object_value["correlation_id"] = MakeString(request.correlation_id);

  ulak::json::Value payload = MakeObject();
  payload.object_value["command"] = MakeString(request.command);
  payload.object_value["target"] = MakeString(request.target);

  ulak::json::Value params = request.params;
  if (params.type == ulak::json::Type::kNull) {
    params = MakeObject();
  }
  payload.object_value["params"] = std::move(params);

  root.object_value["payload"] = std::move(payload);
  return ulak::json::Serialize(root);
}

const char* ToString(CommandParseError error) {
  switch (error) {
    case CommandParseError::kNone:
      return "none";
    case CommandParseError::kInvalidJson:
      return "invalid_json";
    case CommandParseError::kMissingField:
      return "missing_field";
    case CommandParseError::kInvalidType:
      return "invalid_type";
    case CommandParseError::kInvalidValue:
      return "invalid_value";
    case CommandParseError::kUnsupportedSchema:
      return "unsupported_schema";
    case CommandParseError::kUnsupportedCategory:
      return "unsupported_category";
  }
  return "unknown_error";
}

}  // namespace ulak::models
