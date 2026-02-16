#pragma once

#include <string>

namespace ulak::models {

enum class StreamMode {
  kOff,
  kOutputsOnly,
  kCompressedLive,
  kRawDebug
};

bool ParseStreamMode(const std::string& value, StreamMode* out);
std::string ToString(StreamMode mode);

}  // namespace ulak::models
