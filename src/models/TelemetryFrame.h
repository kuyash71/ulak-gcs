#pragma once

#include <optional>
#include <string>

namespace ulak::models {

struct Vector3 {
  double x{0.0};
  double y{0.0};
  double z{0.0};
};

struct Attitude {
  double roll{0.0};
  double pitch{0.0};
  double yaw{0.0};
};

// Station-facing telemetry payload (base + optional simulator/vehicle fields).
struct TelemetryFrame {
  std::string schema_version{"1.0.0"};
  std::string telemetry_type;  // "vehicle" or "simulator"
  std::string frame_id;

  Vector3 position_m;
  Vector3 velocity_mps;
  Attitude attitude_deg;

  // Optional geodetic fields (vehicle payloads).
  std::optional<double> lat_deg;
  std::optional<double> lon_deg;
  std::optional<double> alt_m;

  // Optional simulator-specific fields.
  std::optional<std::string> sim_backend;
  std::optional<std::string> raw_source_frame;
  std::optional<Vector3> raw_position_m;
  std::optional<std::string> transform_id;
};

}  // namespace ulak::models
