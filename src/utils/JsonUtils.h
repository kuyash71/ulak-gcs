#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace ulak::json {

enum class Type {
  kNull,
  kBool,
  kNumber,
  kString,
  kArray,
  kObject
};

struct Value {
  Type type{Type::kNull};
  bool bool_value{false};
  double number_value{0.0};
  std::string string_value;
  std::vector<Value> array_value;
  std::unordered_map<std::string, Value> object_value;
};

// Parses JSON into a Value. Returns false and fills error_message on failure.
bool Parse(const std::string& input, Value* out, std::string* error_message);

// Serializes a Value into JSON with deterministic key ordering.
std::string Serialize(const Value& value);

// Helper accessors for object values.
const Value* GetObjectField(const Value& object, const std::string& key);
bool GetStringField(const Value& object, const std::string& key, std::string* out);
bool GetNumberField(const Value& object, const std::string& key, double* out);
bool GetBoolField(const Value& object, const std::string& key, bool* out);

// Converts numeric value to int with basic range checks.
bool ToInt(const Value& value, int* out);

}  // namespace ulak::json
