#include "JsonUtils.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <limits>
#include <sstream>

namespace ulak::json {
namespace {

class Parser {
 public:
  explicit Parser(const std::string& input) : input_(input) {}

  bool Parse(Value* out) {
    SkipWhitespace();
    if (!ParseValue(out)) {
      return false;
    }
    SkipWhitespace();
    if (!AtEnd()) {
      return Fail("Unexpected trailing characters");
    }
    return true;
  }

  const std::string& error() const {
    return error_;
  }

 private:
  static constexpr int kMaxDepth = 128;

  bool ParseValue(Value* out, int depth = 0) {
    if (depth > kMaxDepth) {
      return Fail("JSON nesting exceeds supported depth");
    }
    if (AtEnd()) {
      return Fail("Unexpected end of input");
    }

    const char token = Peek();
    if (token == '{') {
      return ParseObject(out, depth + 1);
    }
    if (token == '[') {
      return ParseArray(out, depth + 1);
    }
    if (token == '"') {
      out->type = Type::kString;
      return ParseString(&out->string_value);
    }
    if (token == 't') {
      out->type = Type::kBool;
      if (ConsumeLiteral("true")) {
        out->bool_value = true;
        return true;
      }
      return Fail("Invalid literal, expected 'true'");
    }
    if (token == 'f') {
      out->type = Type::kBool;
      if (ConsumeLiteral("false")) {
        out->bool_value = false;
        return true;
      }
      return Fail("Invalid literal, expected 'false'");
    }
    if (token == 'n') {
      out->type = Type::kNull;
      if (ConsumeLiteral("null")) {
        return true;
      }
      return Fail("Invalid literal, expected 'null'");
    }
    if (token == '-' || IsDigit(token)) {
      out->type = Type::kNumber;
      return ParseNumber(&out->number_value);
    }

    return Fail("Unexpected token while parsing value");
  }

  bool ParseObject(Value* out, int depth) {
    if (!ConsumeChar('{')) {
      return Fail("Expected '{'");
    }
    out->type = Type::kObject;
    out->object_value.clear();

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
      Value value;
      if (!ParseValue(&value, depth)) {
        return false;
      }
      out->object_value.emplace(std::move(key), std::move(value));

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

  bool ParseArray(Value* out, int depth) {
    if (!ConsumeChar('[')) {
      return Fail("Expected '['");
    }
    out->type = Type::kArray;
    out->array_value.clear();

    SkipWhitespace();
    if (ConsumeChar(']')) {
      return true;
    }

    while (true) {
      Value value;
      if (!ParseValue(&value, depth)) {
        return false;
      }
      out->array_value.push_back(std::move(value));

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
      if (AtEnd() || !IsHexDigit(Peek())) {
        return Fail("Invalid unicode escape sequence");
      }
      ++position_;
    }
    return true;
  }

  bool ParseNumber(double* out) {
    const size_t start = position_;
    if (ConsumeChar('-')) {
      if (AtEnd()) {
        return Fail("Invalid number format");
      }
    }

    if (ConsumeChar('0')) {
      // Leading zero allowed only as a single digit.
    } else if (IsNonZeroDigit(Peek())) {
      while (!AtEnd() && IsDigit(Peek())) {
        ++position_;
      }
    } else {
      return Fail("Invalid number format");
    }

    if (ConsumeChar('.')) {
      if (AtEnd() || !IsDigit(Peek())) {
        return Fail("Expected digits after decimal point");
      }
      while (!AtEnd() && IsDigit(Peek())) {
        ++position_;
      }
    }

    if (!AtEnd() && (Peek() == 'e' || Peek() == 'E')) {
      ++position_;
      if (!AtEnd() && (Peek() == '+' || Peek() == '-')) {
        ++position_;
      }
      if (AtEnd() || !IsDigit(Peek())) {
        return Fail("Expected digits in exponent");
      }
      while (!AtEnd() && IsDigit(Peek())) {
        ++position_;
      }
    }

    const std::string number_token = input_.substr(start, position_ - start);
    char* end = nullptr;
    const double value = std::strtod(number_token.c_str(), &end);
    if (end == number_token.c_str()) {
      return Fail("Invalid number value");
    }
    *out = value;
    return true;
  }

  bool ConsumeLiteral(const std::string& literal) {
    if (position_ + literal.size() > input_.size()) {
      return false;
    }
    if (input_.compare(position_, literal.size(), literal) != 0) {
      return false;
    }
    position_ += literal.size();
    return true;
  }

  bool ConsumeChar(char expected) {
    if (AtEnd() || Peek() != expected) {
      return false;
    }
    ++position_;
    return true;
  }

  void SkipWhitespace() {
    while (!AtEnd()) {
      const char c = Peek();
      if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
        ++position_;
        continue;
      }
      break;
    }
  }

  bool AtEnd() const {
    return position_ >= input_.size();
  }

  char Peek() const {
    return input_[position_];
  }

  bool Fail(const std::string& message) {
    if (error_.empty()) {
      error_ = message + " at byte " + std::to_string(position_);
    }
    return false;
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

  const std::string& input_;
  size_t position_{0};
  std::string error_;
};

}  // namespace

bool Parse(const std::string& input, Value* out, std::string* error_message) {
  if (!out) {
    if (error_message) {
      *error_message = "Output pointer is null";
    }
    return false;
  }

  Parser parser(input);
  if (!parser.Parse(out)) {
    if (error_message) {
      *error_message = parser.error();
    }
    return false;
  }
  return true;
}

namespace {

void AppendEscapedString(const std::string& input, std::string* out) {
  out->push_back('"');
  for (unsigned char c : input) {
    switch (c) {
      case '"':
        out->append("\\\"");
        break;
      case '\\':
        out->append("\\\\");
        break;
      case '\b':
        out->append("\\b");
        break;
      case '\f':
        out->append("\\f");
        break;
      case '\n':
        out->append("\\n");
        break;
      case '\r':
        out->append("\\r");
        break;
      case '\t':
        out->append("\\t");
        break;
      default:
        if (c < 0x20) {
          static const char kHex[] = "0123456789ABCDEF";
          out->append("\\u00");
          out->push_back(kHex[(c >> 4) & 0xF]);
          out->push_back(kHex[c & 0xF]);
        } else {
          out->push_back(static_cast<char>(c));
        }
        break;
    }
  }
  out->push_back('"');
}

void SerializeValue(const Value& value, std::string* out) {
  switch (value.type) {
    case Type::kNull:
      out->append("null");
      return;
    case Type::kBool:
      out->append(value.bool_value ? "true" : "false");
      return;
    case Type::kNumber: {
      std::ostringstream number;
      number << std::setprecision(15) << value.number_value;
      out->append(number.str());
      return;
    }
    case Type::kString:
      AppendEscapedString(value.string_value, out);
      return;
    case Type::kArray: {
      out->push_back('[');
      for (size_t i = 0; i < value.array_value.size(); ++i) {
        if (i > 0) {
          out->push_back(',');
        }
        SerializeValue(value.array_value[i], out);
      }
      out->push_back(']');
      return;
    }
    case Type::kObject: {
      out->push_back('{');
      std::vector<std::string> keys;
      keys.reserve(value.object_value.size());
      for (const auto& it : value.object_value) {
        keys.push_back(it.first);
      }
      std::sort(keys.begin(), keys.end());
      for (size_t i = 0; i < keys.size(); ++i) {
        if (i > 0) {
          out->push_back(',');
        }
        AppendEscapedString(keys[i], out);
        out->push_back(':');
        SerializeValue(value.object_value.at(keys[i]), out);
      }
      out->push_back('}');
      return;
    }
  }
}

}  // namespace

std::string Serialize(const Value& value) {
  std::string output;
  output.reserve(256);
  SerializeValue(value, &output);
  return output;
}

const Value* GetObjectField(const Value& object, const std::string& key) {
  if (object.type != Type::kObject) {
    return nullptr;
  }
  const auto it = object.object_value.find(key);
  if (it == object.object_value.end()) {
    return nullptr;
  }
  return &it->second;
}

bool GetStringField(const Value& object, const std::string& key, std::string* out) {
  const Value* value = GetObjectField(object, key);
  if (!value || value->type != Type::kString) {
    return false;
  }
  if (out) {
    *out = value->string_value;
  }
  return true;
}

bool GetNumberField(const Value& object, const std::string& key, double* out) {
  const Value* value = GetObjectField(object, key);
  if (!value || value->type != Type::kNumber) {
    return false;
  }
  if (out) {
    *out = value->number_value;
  }
  return true;
}

bool GetBoolField(const Value& object, const std::string& key, bool* out) {
  const Value* value = GetObjectField(object, key);
  if (!value || value->type != Type::kBool) {
    return false;
  }
  if (out) {
    *out = value->bool_value;
  }
  return true;
}

bool ToInt(const Value& value, int* out) {
  if (value.type != Type::kNumber) {
    return false;
  }
  if (value.number_value < static_cast<double>(std::numeric_limits<int>::min()) ||
      value.number_value > static_cast<double>(std::numeric_limits<int>::max())) {
    return false;
  }
  const int candidate = static_cast<int>(value.number_value);
  if (value.number_value != static_cast<double>(candidate)) {
    return false;
  }
  if (out) {
    *out = candidate;
  }
  return true;
}

}  // namespace ulak::json
