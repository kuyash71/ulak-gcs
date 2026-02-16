#include "StreamMode.h"

namespace ulak::models {

bool ParseStreamMode(const std::string& value, StreamMode* out) {
  if (value == "OFF") {
    *out = StreamMode::kOff;
    return true;
  }
  if (value == "OUTPUTS_ONLY") {
    *out = StreamMode::kOutputsOnly;
    return true;
  }
  if (value == "COMPRESSED_LIVE") {
    *out = StreamMode::kCompressedLive;
    return true;
  }
  if (value == "RAW_DEBUG") {
    *out = StreamMode::kRawDebug;
    return true;
  }
  return false;
}

std::string ToString(StreamMode mode) {
  switch (mode) {
    case StreamMode::kOff:
      return "OFF";
    case StreamMode::kOutputsOnly:
      return "OUTPUTS_ONLY";
    case StreamMode::kCompressedLive:
      return "COMPRESSED_LIVE";
    case StreamMode::kRawDebug:
      return "RAW_DEBUG";
  }
  return "OFF";
}

}  // namespace ulak::models
