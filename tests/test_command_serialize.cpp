#include "GcsCommand.h"

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

bool TestParseValid() {
  const std::string json = R"({
    "schema_version": "1.0.0",
    "category": "station/commands/request",
    "timestamp": "2026-02-10T19:00:00Z",
    "source": "station",
    "correlation_id": "abc-123",
    "payload": {
      "command": "STOP_MISSION",
      "target": "companion_computer",
      "params": {}
    }
  })";

  ulak::models::CommandRequest request;
  const auto result = ulak::models::ParseCommandRequest(json, &request);
  return Expect(result.ok, "Expected valid command request to parse") &&
         Expect(request.command == "STOP_MISSION", "Expected command=STOP_MISSION") &&
         Expect(request.target == "companion_computer", "Expected target=companion_computer") &&
         Expect(request.correlation_id == "abc-123", "Expected correlation_id=abc-123");
}

bool TestMissingField() {
  const std::string json = R"({
    "schema_version": "1.0.0",
    "category": "station/commands/request",
    "timestamp": "2026-02-10T19:00:00Z",
    "source": "station",
    "payload": {
      "command": "STOP_MISSION",
      "target": "companion_computer",
      "params": {}
    }
  })";

  ulak::models::CommandRequest request;
  const auto result = ulak::models::ParseCommandRequest(json, &request);
  return Expect(!result.ok, "Expected missing correlation_id to fail") &&
         Expect(result.error == ulak::models::CommandParseError::kMissingField,
                "Expected kMissingField for missing correlation_id");
}

bool TestInvalidJson() {
  const std::string json = R"({"schema_version": "1.0.0", "category": )";
  ulak::models::CommandRequest request;
  const auto result = ulak::models::ParseCommandRequest(json, &request);
  return Expect(!result.ok, "Expected malformed JSON to fail") &&
         Expect(result.error == ulak::models::CommandParseError::kInvalidJson,
                "Expected kInvalidJson for malformed JSON");
}

bool TestInvalidCategory() {
  const std::string json = R"({
    "schema_version": "1.0.0",
    "category": "station/commands/ack",
    "timestamp": "2026-02-10T19:00:00Z",
    "source": "station",
    "correlation_id": "abc-123",
    "payload": {
      "command": "STOP_MISSION",
      "target": "companion_computer",
      "params": {}
    }
  })";

  ulak::models::CommandRequest request;
  const auto result = ulak::models::ParseCommandRequest(json, &request);
  return Expect(!result.ok, "Expected unsupported category to fail") &&
         Expect(result.error == ulak::models::CommandParseError::kUnsupportedCategory,
                "Expected kUnsupportedCategory for non-request category");
}

bool TestExtraFields() {
  const std::string json = R"({
    "schema_version": "1.0.0",
    "category": "station/commands/request",
    "timestamp": "2026-02-10T19:00:00Z",
    "source": "station",
    "correlation_id": "abc-123",
    "extra_root": "ignored",
    "payload": {
      "command": "STOP_MISSION",
      "target": "companion_computer",
      "params": {},
      "extra_payload": "ignored"
    }
  })";

  ulak::models::CommandRequest request;
  const auto result = ulak::models::ParseCommandRequest(json, &request);
  return Expect(result.ok, "Expected extra fields to be ignored");
}

bool TestDeterministicSerialization() {
  ulak::models::CommandRequest request;
  request.schema_version = "1.0.0";
  request.timestamp = "2026-02-10T19:00:00Z";
  request.source = "station";
  request.correlation_id = "abc-123";
  request.command = "STOP_MISSION";
  request.target = "companion_computer";
  request.params.type = ulak::json::Type::kObject;

  const std::string serialized = ulak::models::SerializeCommandRequest(request);
  const std::string expected =
      "{\"category\":\"station/commands/request\","
      "\"correlation_id\":\"abc-123\","
      "\"payload\":{\"command\":\"STOP_MISSION\",\"params\":{},\"target\":\"companion_computer\"},"
      "\"schema_version\":\"1.0.0\","
      "\"source\":\"station\","
      "\"timestamp\":\"2026-02-10T19:00:00Z\"}";

  if (!Expect(serialized == expected, "Expected deterministic serialized JSON")) {
    std::cerr << "[test] Serialized output: " << serialized << '\n';
    return false;
  }
  return true;
}

}  // namespace

int main() {
  const bool ok = TestParseValid() &&
                  TestMissingField() &&
                  TestInvalidJson() &&
                  TestInvalidCategory() &&
                  TestExtraFields() &&
                  TestDeterministicSerialization();

  if (!ok) {
    return 1;
  }

  std::cout << "[test] Command serialization tests passed.\n";
  return 0;
}
