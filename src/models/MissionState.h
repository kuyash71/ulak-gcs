#pragma once

#include <string>

namespace ulak::models {

struct MissionState {
  std::string schema_version{"1.0.0"};
  std::string state;
  double progress{0.0};
  std::string timestamp;
  std::string note;
};

}  // namespace ulak::models
