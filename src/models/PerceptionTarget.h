#pragma once

#include <string>

namespace ulak::models {

struct TargetDescriptor {
  std::string color;
  std::string shape;
};

struct Alignment {
  double dx{0.0};
  double dy{0.0};
};

struct PerceptionTarget {
  std::string schema_version{"1.0.0"};
  TargetDescriptor target;
  Alignment alignment;
  double confidence{0.0};
  std::string timestamp;
};

}  // namespace ulak::models
