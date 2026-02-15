#include "BootstrapConfig.h"

#include <cctype>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>

namespace ulak::bootstrap {
namespace {

enum class JsonType {
  kObject,
  kArray,
  kString,
  kNumber,
  kBoolean,
  kNull,
};

struct JsonValueSummary {
  JsonType type{JsonType::kNull};
  std::string string_value;
};

class JsonParser {
 public:
  explicit JsonParser(std::string_view input) : input_(input) {}

  bool ParseRootObject(std::unordered_map<std::string, JsonValueSummary>* root_fields) {
    SkipWhitespace();
    if (!ParseObject(/*depth=*/0, root_fields)) {
      return false;
    }

    SkipWhitespace();
    if (!AtEnd()) {
      return Fail("Unexpected trailing characters");
    }
    return true;
  }

  const std::string& error_message() const {
    return error_message_;
  }

 private:
  static constexpr int kMaxJsonDepth = 128;

  bool ParseObject(int depth, std::unordered_map<std::string, JsonValueSummary>* root_fields) {
    if (depth > kMaxJsonDepth) {
      return Fail("JSON nesting exceeds supported depth");
    }

    if (!ConsumeChar('{')) {
      return Fail("Expected '{'");
    }

    SkipWhitespace();
    if (ConsumeChar('}')) {
      return true;
    }

    while (true) {
      std::string key;
      if (!ParseString(&key)) {
        return false;
      }

      SkipWhitespace();
      if (!ConsumeChar(':')) {
        return Fail("Expected ':' after object key");
      }

      SkipWhitespace();
      JsonValueSummary summary;
      if (!ParseValue(depth + 1, &summary)) {
        return false;
      }

      if (depth == 0 && root_fields != nullptr) {
        (*root_fields)[std::move(key)] = std::move(summary);
      }

      SkipWhitespace();
      if (ConsumeChar(',')) {
        SkipWhitespace();
        continue;
      }

      if (ConsumeChar('}')) {
        return true;
      }

      return Fail("Expected ',' or '}' in object");
    }
  }

  bool ParseArray(int depth) {
    if (depth > kMaxJsonDepth) {
      return Fail("JSON nesting exceeds supported depth");
    }

    if (!ConsumeChar('[')) {
      return Fail("Expected '['");
    }

    SkipWhitespace();
    if (ConsumeChar(']')) {
      return true;
    }

    while (true) {
      JsonValueSummary ignored;
      if (!ParseValue(depth + 1, &ignored)) {
        return false;
      }

      SkipWhitespace();
      if (ConsumeChar(',')) {
        SkipWhitespace();
        continue;
      }

      if (ConsumeChar(']')) {
        return true;
      }

      return Fail("Expected ',' or ']' in array");
    }
  }

  bool ParseValue(int depth, JsonValueSummary* summary) {
    if (AtEnd()) {
      return Fail("Unexpected end of input while parsing value");
    }

    const char token = PeekChar();

    if (token == '{') {
      summary->type = JsonType::kObject;
      return ParseObject(depth, nullptr);
    }

    if (token == '[') {
      summary->type = JsonType::kArray;
      return ParseArray(depth);
    }

    if (token == '"') {
      summary->type = JsonType::kString;
      return ParseString(&summary->string_value);
    }

    if (token == 't') {
      summary->type = JsonType::kBoolean;
      if (ConsumeLiteral("true")) {
        return true;
      }
      return Fail("Invalid literal, expected 'true'");
    }

    if (token == 'f') {
      summary->type = JsonType::kBoolean;
      if (ConsumeLiteral("false")) {
        return true;
      }
      return Fail("Invalid literal, expected 'false'");
    }

    if (token == 'n') {
      summary->type = JsonType::kNull;
      if (ConsumeLiteral("null")) {
        return true;
      }
      return Fail("Invalid literal, expected 'null'");
    }

    if (token == '-' || IsDigit(token)) {
      summary->type = JsonType::kNumber;
      return ParseNumber();
    }

    return Fail("Unexpected token while parsing value");
  }

  bool ParseString(std::string* output) {
    if (!ConsumeChar('"')) {
      return Fail("Expected '\"' to start string");
    }

    while (!AtEnd()) {
      const char c = input_[position_++];

      if (c == '"') {
        return true;
      }

      if (static_cast<unsigned char>(c) < 0x20) {
        return Fail("Unescaped control character in string");
      }

      if (c != '\\') {
        output->push_back(c);
        continue;
      }

      if (AtEnd()) {
        return Fail("Unterminated escape sequence");
      }

      const char escaped = input_[position_++];
      switch (escaped) {
        case '"':
        case '\\':
        case '/':
          output->push_back(escaped);
          break;
        case 'b':
          output->push_back('\b');
          break;
        case 'f':
          output->push_back('\f');
          break;
        case 'n':
          output->push_back('\n');
          break;
        case 'r':
          output->push_back('\r');
          break;
        case 't':
          output->push_back('\t');
          break;
        case 'u':
          if (!ParseUnicodeEscape()) {
            return false;
          }
          output->push_back('?');
          break;
        default:
          return Fail("Invalid escape sequence in string");
      }
    }

    return Fail("Unterminated string");
  }

  bool ParseUnicodeEscape() {
    for (int i = 0; i < 4; ++i) {
      if (AtEnd() || !IsHexDigit(PeekChar())) {
        return Fail("Invalid unicode escape sequence");
      }
      ++position_;
    }
    return true;
  }

  bool ParseNumber() {
    ConsumeChar('-');

    if (AtEnd()) {
      return Fail("Incomplete number");
    }

    if (ConsumeChar('0')) {
      // Leading zero is valid only as the single integer digit.
    } else if (IsNonZeroDigit(PeekChar())) {
      while (!AtEnd() && IsDigit(PeekChar())) {
        ++position_;
      }
    } else {
      return Fail("Invalid number format");
    }

    if (ConsumeChar('.')) {
      if (AtEnd() || !IsDigit(PeekChar())) {
        return Fail("Expected digits after decimal point");
      }
      while (!AtEnd() && IsDigit(PeekChar())) {
        ++position_;
      }
    }

    if (!AtEnd() && (PeekChar() == 'e' || PeekChar() == 'E')) {
      ++position_;
      if (!AtEnd() && (PeekChar() == '+' || PeekChar() == '-')) {
        ++position_;
      }
      if (AtEnd() || !IsDigit(PeekChar())) {
        return Fail("Expected digits in exponent");
      }
      while (!AtEnd() && IsDigit(PeekChar())) {
        ++position_;
      }
    }

    return true;
  }

  bool ConsumeLiteral(std::string_view literal) {
    if (position_ + literal.size() > input_.size()) {
      return false;
    }

    if (input_.substr(position_, literal.size()) != literal) {
      return false;
    }

    position_ += literal.size();
    return true;
  }

  void SkipWhitespace() {
    while (!AtEnd()) {
      const char c = PeekChar();
      if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
        ++position_;
        continue;
      }
      break;
    }
  }

  bool ConsumeChar(char expected) {
    if (AtEnd() || PeekChar() != expected) {
      return false;
    }
    ++position_;
    return true;
  }

  bool Fail(const std::string& message) {
    if (error_message_.empty()) {
      error_message_ = message + " at byte " + std::to_string(position_);
    }
    return false;
  }

  bool AtEnd() const {
    return position_ >= input_.size();
  }

  char PeekChar() const {
    return input_[position_];
  }

  static bool IsDigit(char c) {
    return c >= '0' && c <= '9';
  }

  static bool IsNonZeroDigit(char c) {
    return c >= '1' && c <= '9';
  }

  static bool IsHexDigit(char c) {
    return std::isxdigit(static_cast<unsigned char>(c)) != 0;
  }

  std::string_view input_;
  size_t position_{0};
  std::string error_message_;
};

ValidationResult MakeError(ValidationError error, std::string message) {
  ValidationResult result;
  result.ok = false;
  result.error = error;
  result.message = std::move(message);
  return result;
}

bool IsObjectField(const std::unordered_map<std::string, JsonValueSummary>& fields,
                   const std::string& key) {
  const auto it = fields.find(key);
  if (it == fields.end()) {
    return false;
  }
  return it->second.type == JsonType::kObject;
}

}  // namespace

ValidationResult ValidateConfigFile(const std::filesystem::path& config_path) {
  std::error_code ec;
  if (!std::filesystem::exists(config_path, ec)) {
    return MakeError(ValidationError::kMissingFile,
                     "Config file does not exist: " + config_path.string());
  }
  if (ec) {
    return MakeError(ValidationError::kUnreadableFile,
                     "Failed to check config file: " + ec.message());
  }

  if (std::filesystem::is_directory(config_path, ec)) {
    return MakeError(ValidationError::kUnreadableFile,
                     "Config path points to a directory: " + config_path.string());
  }
  if (ec) {
    return MakeError(ValidationError::kUnreadableFile,
                     "Failed to inspect config path: " + ec.message());
  }

  std::ifstream input(config_path);
  if (!input.good()) {
    return MakeError(ValidationError::kUnreadableFile,
                     "Config file cannot be opened: " + config_path.string());
  }

  std::ostringstream buffer;
  buffer << input.rdbuf();
  if (input.bad()) {
    return MakeError(ValidationError::kUnreadableFile,
                     "Config file cannot be read completely: " + config_path.string());
  }

  const std::string content = buffer.str();
  if (content.empty()) {
    return MakeError(ValidationError::kInvalidJson, "Config file is empty");
  }

  std::unordered_map<std::string, JsonValueSummary> root_fields;
  JsonParser parser(content);
  if (!parser.ParseRootObject(&root_fields)) {
    return MakeError(ValidationError::kInvalidJson, parser.error_message());
  }

  auto schema_it = root_fields.find("schema_version");
  if (schema_it == root_fields.end()) {
    return MakeError(ValidationError::kMissingRequiredField,
                     "Required field 'schema_version' is missing");
  }
  if (schema_it->second.type != JsonType::kString) {
    return MakeError(ValidationError::kInvalidFieldType,
                     "Field 'schema_version' must be a string");
  }
  if (schema_it->second.string_value.empty()) {
    return MakeError(ValidationError::kInvalidFieldValue,
                     "Field 'schema_version' must not be empty");
  }

  auto profile_it = root_fields.find("active_profile");
  if (profile_it == root_fields.end()) {
    return MakeError(ValidationError::kMissingRequiredField,
                     "Required field 'active_profile' is missing");
  }
  if (profile_it->second.type != JsonType::kString) {
    return MakeError(ValidationError::kInvalidFieldType,
                     "Field 'active_profile' must be a string");
  }
  if (profile_it->second.string_value.empty()) {
    return MakeError(ValidationError::kInvalidFieldValue,
                     "Field 'active_profile' must not be empty");
  }

  if (!IsObjectField(root_fields, "telemetry")) {
    return MakeError(ValidationError::kMissingRequiredField,
                     "Required object field 'telemetry' is missing");
  }
  if (!IsObjectField(root_fields, "companion")) {
    return MakeError(ValidationError::kMissingRequiredField,
                     "Required object field 'companion' is missing");
  }
  if (!IsObjectField(root_fields, "stream")) {
    return MakeError(ValidationError::kMissingRequiredField,
                     "Required object field 'stream' is missing");
  }

  ValidationResult result;
  result.ok = true;
  result.error = ValidationError::kNone;
  result.message = "Config validated successfully";
  result.schema_version = schema_it->second.string_value;
  result.active_profile = profile_it->second.string_value;
  return result;
}

const char* ToString(ValidationError error) {
  switch (error) {
    case ValidationError::kNone:
      return "none";
    case ValidationError::kMissingFile:
      return "missing_file";
    case ValidationError::kUnreadableFile:
      return "unreadable_file";
    case ValidationError::kInvalidJson:
      return "invalid_json";
    case ValidationError::kMissingRequiredField:
      return "missing_required_field";
    case ValidationError::kInvalidFieldType:
      return "invalid_field_type";
    case ValidationError::kInvalidFieldValue:
      return "invalid_field_value";
  }
  return "unknown_error";
}

}  // namespace ulak::bootstrap
