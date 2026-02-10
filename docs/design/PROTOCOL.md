# ULAK GCS - Protocol Contract (MVP)

## 1. Purpose

This document defines the MVP message protocol contract between:

- ULAK GCS Station (operator app),
- Flight Controller (FC),
- Companion Computer (CC).

It is the wire-level contract for message categories, envelope fields, command lifecycle semantics, error handling, and retry/idempotency behavior.

## 2. Scope and assumptions

- Transport is implementation-specific (serial/UDP/TCP/MQTT/WebSocket). The payload contract remains the same.
- Message format is JSON (UTF-8).
- Timestamp format is RFC3339 UTC (example: `2026-02-10T18:45:30Z`).
- The architecture is hybrid:
  - FC is the primary telemetry source.
  - CC is the mission/perception/stream/safety source.
  - Station is the command originator and visibility surface.
- Telemetry payloads use a shared XYZ-based base schema for both vehicle and simulator feeds.
- For `vehicle` telemetry, geodetic fields (`lat_deg`, `lon_deg`, `alt_m`) are optional extensions and not the canonical position contract.
- For simulation telemetry, a configurable coordinate transformer is required because Gazebo and ArduPilot local XYZ conventions are not identical.

## 3. Protocol versioning

- `schema_version` MUST be present in every message.
- MVP baseline version is `1.0.0`.
- Major version mismatch (`1.x.x` vs `2.x.x`) MUST be rejected.
- Minor/patch additions MAY be accepted only if unknown fields are safely ignored.

## 4. Message categories

The following categories are mandatory for MVP:

- `telemetry/*`
- `mission/state`
- `perception/output`
- `safety/events`
- `station/commands`

Recommended concrete category values:

- `telemetry/vehicle`
- `telemetry/simulator`
- `telemetry/health`
- `mission/state`
- `perception/output`
- `safety/events`
- `station/commands/request`
- `station/commands/ack`
- `station/commands/reject`
- `station/commands/result` (optional but recommended)

## 5. Common envelope (required fields)

Every protocol message MUST follow this envelope:

```json
{
  "schema_version": "1.0.0",
  "category": "station/commands/request",
  "timestamp": "2026-02-10T18:45:30Z",
  "source": "station",
  "correlation_id": "b9b0b8f2-9c46-4fce-8f8c-d7608f3f1e8a",
  "payload": {}
}
```

Required fields:

- `schema_version` (string): protocol schema version.
- `category` (string): message category.
- `timestamp` (string): RFC3339 UTC generation time.
- `source` (string enum): `station` | `flight_controller` | `companion_computer`.
- `correlation_id` (string): UUID (v4 recommended), used to correlate lifecycle events.
- `payload` (object): category-specific body.

Validation rules:

- Missing required fields -> reject as `INVALID_SCHEMA`.
- Invalid timestamp format -> reject as `INVALID_SCHEMA`.
- Unknown category -> reject as `UNKNOWN_CATEGORY`.

## 6. Minimal payload contracts

### 6.1 `telemetry/*` (base schema + derived variants)

#### 6.1.1 Base telemetry schema (logical parent)

All telemetry messages (`telemetry/vehicle` and `telemetry/simulator`) MUST include this base payload shape.

Naming contract for sustainability:

- `telemetry/vehicle`: normalized vehicle state from the active flight stack (real airframe, SITL, or HIL).
- `telemetry/simulator`: simulator-oriented telemetry stream that carries transform context for world/frame reconciliation.

```json
{
  "telemetry_type": "vehicle",
  "frame_id": "LOCAL_ENU",
  "position_m": { "x": 12.4, "y": -3.1, "z": 24.8 },
  "velocity_mps": { "x": 1.2, "y": 0.0, "z": -0.3 },
  "attitude_deg": { "roll": 1.1, "pitch": -0.4, "yaw": 87.2 },
  "vehicle_mode": "GUIDED",
  "battery_percent": 78
}
```

Required base fields:

- `telemetry_type`: `vehicle` | `simulator`
- `frame_id`: coordinate frame of `position_m` and `velocity_mps`
- `position_m`: XYZ position in meters (`x`, `y`, `z`)
- `velocity_mps`: XYZ velocity in m/s (`x`, `y`, `z`)
- `attitude_deg`: Euler attitude in degrees

#### 6.1.2 `telemetry/vehicle` (derived)

Real vehicle payloads extend the base schema and MAY include geodetic coordinates.

```json
{
  "telemetry_type": "vehicle",
  "frame_id": "ARDUPILOT_LOCAL_NED",
  "position_m": { "x": 34.5, "y": -10.2, "z": -6.8 },
  "velocity_mps": { "x": 5.2, "y": 0.1, "z": -0.4 },
  "attitude_deg": { "roll": -2.0, "pitch": 1.4, "yaw": 240.0 },
  "vehicle_mode": "GUIDED",
  "battery_percent": 82,
  "geodetic": {
    "lat_deg": 40.742000,
    "lon_deg": 30.332000,
    "alt_m": 112.4
  }
}
```

`geodetic` is optional for protocol compatibility and mapping use-cases, but station-side motion/state logic should be based on base XYZ fields.

#### 6.1.3 `telemetry/simulator` (derived)

Simulation payloads extend the base schema and include raw source frame information.

```json
{
  "telemetry_type": "simulator",
  "sim_backend": "gazebo",
  "frame_id": "ARDUPILOT_LOCAL_NED",
  "position_m": { "x": 11.0, "y": 4.3, "z": -2.5 },
  "velocity_mps": { "x": 0.7, "y": 0.1, "z": -0.2 },
  "attitude_deg": { "roll": 0.0, "pitch": 0.0, "yaw": 30.0 },
  "raw_source_frame": "GAZEBO_WORLD",
  "raw_position_m": { "x": 4.3, "y": 11.0, "z": 2.5 },
  "transform_id": "gazebo_world_to_ardupilot_ned_v1"
}
```

#### 6.1.4 Simulation coordinate transformer (configurable)

The simulation adapter MUST support a configurable frame transformer to align Gazebo XYZ with ArduPilot local XYZ expectations.

Recommended transform config payload:

```json
{
  "transform_id": "gazebo_world_to_ardupilot_ned_v1",
  "enabled": true,
  "source_frame_id": "GAZEBO_WORLD",
  "target_frame_id": "ARDUPILOT_LOCAL_NED",
  "axis_map": { "x": "y", "y": "x", "z": "-z" },
  "translation_m": { "x": 0.0, "y": 0.0, "z": 0.0 },
  "rotation_deg": { "roll": 0.0, "pitch": 0.0, "yaw": 0.0 },
  "scale": { "x": 1.0, "y": 1.0, "z": 1.0 }
}
```

Rules:

- Transformation is applied to raw simulation telemetry before publishing `position_m` and `velocity_mps`.
- Active transform config should be loaded from station simulation settings and be overrideable at runtime.
- Runtime transform updates MUST be audit-logged.

### 6.2 `mission/state`

```json
{
  "state": "MISSION_RUNNING",
  "progress": 0.42,
  "note": "loop 2/4"
}
```

### 6.3 `perception/output`

```json
{
  "target": { "color": "red", "shape": "triangle" },
  "alignment": { "dx": -0.12, "dy": 0.08 },
  "confidence": 0.91
}
```

### 6.4 `safety/events`

```json
{
  "severity": "WARN",
  "code": "VISION_LOST",
  "message": "Perception input missing for 1.2s",
  "recommended_action": "HOLD"
}
```

`severity` values for MVP:

- `WARN`
- `ERROR`
- `CRITICAL`

### 6.5 `station/commands/request`

```json
{
  "command": "START_MISSION",
  "target": "companion_computer",
  "params": {
    "mission_id": "mission_1"
  }
}
```

Command names (MVP set):

- `START_MISSION`
- `STOP_MISSION`
- `SET_PARAM`
- `SET_SIMULATOR_COORD_TRANSFORM`
- `PANIC_RTL`

`PANIC_RTL` is fixed behavior and MUST map to RTL action regardless of profile.

## 7. Command lifecycle semantics (ACK/REJECT/TIMEOUT)

### 7.1 Request

- Station publishes `station/commands/request`.
- `correlation_id` MUST be unique per new logical command.
- If a request is retried, it MUST reuse the same `correlation_id`.

### 7.2 ACK

Receiver sends `station/commands/ack` when request is accepted for execution.

ACK means:

- request is syntactically valid,
- target accepted execution responsibility.

ACK does NOT mean command completed successfully.

ACK payload:

```json
{
  "status": "ACK",
  "accepted_by": "flight_controller"
}
```

### 7.3 REJECT

Receiver sends `station/commands/reject` when request cannot be accepted.

REJECT payload:

```json
{
  "status": "REJECT",
  "error_code": "SAFETY_CONSTRAINT",
  "message": "Command blocked by safety policy"
}
```

### 7.4 TIMEOUT

Two timeout classes exist:

- `ACK_TIMEOUT`: no ACK or REJECT received in `T_ack`.
- `EXEC_TIMEOUT`: ACK received, but no completion evidence by `T_exec`.

MVP default timers:

- `T_ack = 2s`
- `T_exec = 10s` (command-specific overrides allowed)

Completion evidence:

- `station/commands/result` with `SUCCESS` or `FAILED`, or
- deterministic state/event transition proving completion (example: mission enters expected state).

ACK without completion evidence MUST NOT be treated as completion.

## 8. Error codes

Mandatory error codes for MVP:

- `INVALID_SCHEMA`: missing/invalid envelope or payload fields.
- `UNKNOWN_CATEGORY`: category is not recognized.
- `UNSUPPORTED_COMMAND`: command is not implemented for target.
- `INVALID_STATE`: command is valid but not allowed in current system state.
- `SAFETY_CONSTRAINT`: blocked by policy/safety layer.
- `AUTHORIZATION_FAILED`: source is not authorized.
- `RATE_LIMITED`: rejected due to command throttling.
- `TARGET_BUSY`: target temporarily cannot accept command.
- `TARGET_UNREACHABLE`: route/link unavailable.
- `INTERNAL_ERROR`: unhandled internal execution failure.
- `DUPLICATE_CORRELATION_ID`: duplicate new-command key usage.
- `ACK_TIMEOUT`: generated by station lifecycle logic.
- `EXEC_TIMEOUT`: generated by station lifecycle logic.

## 9. Retry and idempotency rules

Idempotency:

- `correlation_id` is the idempotency key for command requests.
- Receiver MUST deduplicate repeated requests with the same `correlation_id` for a minimum 60s window.
- For duplicates, receiver MUST return the same terminal response (`ACK`, `REJECT`, or `result`) when available.

Retry policy:

- Automatic retry is allowed only for transient conditions:
  - `TARGET_UNREACHABLE`
  - `TARGET_BUSY`
  - `INTERNAL_ERROR`
  - `ACK_TIMEOUT`
- Retry schedule (default): up to 3 attempts with exponential backoff `500ms`, `1s`, `2s`.
- Do NOT auto-retry:
  - `INVALID_SCHEMA`
  - `UNKNOWN_CATEGORY`
  - `UNSUPPORTED_COMMAND`
  - `INVALID_STATE`
  - `SAFETY_CONSTRAINT`
  - `AUTHORIZATION_FAILED`
  - `RATE_LIMITED`
- `PANIC_RTL` can be retried only by explicit operator action (no silent background retry).

## 10. Example command flow

### 10.1 Request

```json
{
  "schema_version": "1.0.0",
  "category": "station/commands/request",
  "timestamp": "2026-02-10T19:00:00Z",
  "source": "station",
  "correlation_id": "2cf42dca-d8a2-46d2-bdfd-677ee6a66e8f",
  "payload": {
    "command": "STOP_MISSION",
    "target": "companion_computer",
    "params": {}
  }
}
```

### 10.2 ACK

```json
{
  "schema_version": "1.0.0",
  "category": "station/commands/ack",
  "timestamp": "2026-02-10T19:00:00Z",
  "source": "companion_computer",
  "correlation_id": "2cf42dca-d8a2-46d2-bdfd-677ee6a66e8f",
  "payload": {
    "status": "ACK",
    "accepted_by": "companion_computer"
  }
}
```

### 10.3 Result (optional recommended pattern)

```json
{
  "schema_version": "1.0.0",
  "category": "station/commands/result",
  "timestamp": "2026-02-10T19:00:01Z",
  "source": "companion_computer",
  "correlation_id": "2cf42dca-d8a2-46d2-bdfd-677ee6a66e8f",
  "payload": {
    "status": "SUCCESS"
  }
}
```

### 10.4 `telemetry/simulator` example

```json
{
  "schema_version": "1.0.0",
  "category": "telemetry/simulator",
  "timestamp": "2026-02-10T19:00:05Z",
  "source": "companion_computer",
  "correlation_id": "a6f6a5a8-a7ef-4ce2-bf53-1f7232acc9f0",
  "payload": {
    "telemetry_type": "simulator",
    "sim_backend": "gazebo",
    "frame_id": "ARDUPILOT_LOCAL_NED",
    "position_m": { "x": 11.0, "y": 4.3, "z": -2.5 },
    "velocity_mps": { "x": 0.7, "y": 0.1, "z": -0.2 },
    "attitude_deg": { "roll": 0.0, "pitch": 0.0, "yaw": 30.0 },
    "raw_source_frame": "GAZEBO_WORLD",
    "raw_position_m": { "x": 4.3, "y": 11.0, "z": 2.5 },
    "transform_id": "gazebo_world_to_ardupilot_ned_v1"
  }
}
```

### 10.5 Runtime transform update command example

```json
{
  "schema_version": "1.0.0",
  "category": "station/commands/request",
  "timestamp": "2026-02-10T19:00:10Z",
  "source": "station",
  "correlation_id": "f9fdaf46-bb99-493e-a6aa-61857e9f3f5e",
  "payload": {
    "command": "SET_SIMULATOR_COORD_TRANSFORM",
    "target": "companion_computer",
    "params": {
      "transform_id": "gazebo_world_to_ardupilot_ned_v2",
      "source_frame_id": "GAZEBO_WORLD",
      "target_frame_id": "ARDUPILOT_LOCAL_NED",
      "axis_map": { "x": "y", "y": "x", "z": "-z" },
      "translation_m": { "x": 0.0, "y": 0.0, "z": 0.0 },
      "rotation_deg": { "roll": 0.0, "pitch": 0.0, "yaw": 0.0 },
      "scale": { "x": 1.0, "y": 1.0, "z": 1.0 }
    }
  }
}
```

---

This contract is the MVP baseline. Future versions can extend payload fields and categories without breaking envelope invariants.
