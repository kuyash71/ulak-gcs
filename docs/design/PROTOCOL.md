# ULAK GCS - Protocol Contract (MVP)

## 1. Purpose

This document defines the MVP message protocol contract between:

- ULAK GCS Station (operator app),
- Flight Controller / FC (Pixhawk / ArduPilot),
- Companion Computer / CC (Raspberry Pi 4B).

It is the wire-level contract for message categories, envelope fields, command routing,
command lifecycle semantics, error handling, and retry/idempotency behavior.

## 2. Scope and assumptions

- Transport is deployment-specific (see Section 2.1). The payload contract remains the same.
- Message format is JSON (UTF-8), except for MAVLink channels which use the MAVLink binary protocol.
- Timestamp format is RFC3339 UTC (example: `2026-02-10T18:45:30Z`).
- The architecture is hybrid:
  - FC is the primary telemetry source.
  - CC is the mission/perception/stream/safety source.
  - Station is the command originator and visibility surface.
- Telemetry payloads use a shared XYZ-based base schema for both vehicle and simulator feeds.
- For `vehicle` telemetry, geodetic fields (`lat_deg`, `lon_deg`, `alt_m`) are optional extensions
  and not the canonical position contract.
- For simulation telemetry, a configurable coordinate transformer is required because Gazebo and
  ArduPilot local XYZ conventions are not identical.

### 2.1 Command routing

GCS command responsibility is minimal and explicit:

| Command | Target | Transport |
|---------|--------|-----------|
| `PANIC_RTL` | Pixhawk (FC) | MAVLink/UDP — `MAV_CMD_NAV_RETURN_TO_LAUNCH` (cmd 20) |
| `START_MISSION` | Raspberry Pi (CC) | TCP JSON |
| `STOP_MISSION` | Raspberry Pi (CC) | TCP JSON |
| `SET_PARAM` | Raspberry Pi (CC) | TCP JSON |
| `SET_SIMULATOR_COORD_TRANSFORM` | Raspberry Pi (CC) | TCP JSON (simulation mode only) |

Pi–Pixhawk communication is fully independent of GCS. The GCS does not participate in,
observe, or control the Pi–Pixhawk link. The GCS only sends `PANIC_RTL` directly to the
Pixhawk via MAVLink.

### 2.2 Deployment modes and transport selection

The station operates in one of two modes set by `deployment_mode` in `settings.json`.
The mode determines the transport for **all** data channels.
The JSON protocol described in this document applies to the `real` mode only.
In `simulation` mode, ROS2 message types replace the JSON envelope (see Section 11).

#### Mode: `simulation`

All data flows via ROS2 topics through a rosbridge WebSocket connection.
There is no direct UDP or TCP connection to any component.

| Channel | Transport | Config key |
|---------|-----------|------------|
| Telemetry | ROS2 topic (rosbridge WS) | `ros2.topics.telemetry` |
| Vehicle state | ROS2 topic | `ros2.topics.vehicle_state` |
| Mission state | ROS2 topic | `ros2.topics.mission_state` |
| Perception output | ROS2 topic | `ros2.topics.perception_output` |
| Safety events | ROS2 topic | `ros2.topics.safety_events` |
| Camera | ROS2 topic | `ros2.topics.camera` |
| Commands | ROS2 topic | `ros2.topics.commands` |

rosbridge endpoint: `ros2.bridge.host` + `ros2.bridge.port`.
On Windows/WSL2 the host MUST be the WSL2 NIC IP (e.g. `172.x.x.x`), not `127.0.0.1`.

#### Mode: `real`

All data flows over direct network connections over Wi-Fi. No ROS2 dependency.

| Channel | Transport | Config key |
|---------|-----------|------------|
| Telemetry (FC) | MAVLink / UDP via WiFi telemetry module | `network.telemetry_endpoint` |
| Mission / Perception / Safety (CC) | TCP JSON | `network.companion_endpoint` |
| Commands to CC | TCP JSON | `network.command_endpoint` |
| Commands to FC (`PANIC_RTL` only) | MAVLink / UDP | `network.telemetry_endpoint` (same link) |
| Camera stream | H.264 TCP/UDP | `network.stream` |

## 3. Protocol versioning

- `schema_version` MUST be present in every JSON message.
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
- `station/commands/result` (post-MVP, optional)

## 5. Common envelope (required fields)

Every JSON protocol message MUST follow this envelope:

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

- Missing required fields → reject as `INVALID_SCHEMA`.
- Invalid timestamp format → reject as `INVALID_SCHEMA`.
- Unknown category → reject as `UNKNOWN_CATEGORY`.

## 6. Minimal payload contracts

### 6.1 `telemetry/*` (base schema + derived variants)

#### 6.1.1 Base telemetry schema

All telemetry messages (`telemetry/vehicle` and `telemetry/simulator`) MUST include this base payload shape.

Naming contract:

- `telemetry/vehicle`: normalized vehicle state from the active flight stack.
- `telemetry/simulator`: simulator-oriented telemetry stream that carries transform context
  for world/frame reconciliation.

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
- `vehicle_mode`: active flight mode string (e.g. `GUIDED`, `RTL`, `LOITER`)

`vehicle_mode` is the primary mechanism for confirming `PANIC_RTL` delivery —
when this field transitions to `RTL`, the command is considered confirmed.

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

`geodetic` is optional. Station-side state logic must be based on base XYZ fields.

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

The simulation component MUST support a configurable frame transformer to align
Gazebo XYZ with ArduPilot local XYZ expectations.

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

- Transformation is applied to raw simulation telemetry before publishing `position_m`
  and `velocity_mps`.
- Active transform config is loaded from station simulation settings.
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

`severity` values for MVP: `WARN`, `ERROR`, `CRITICAL`.

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

Command routing table (MVP):

| Command | `target` value | Transport |
|---------|---------------|-----------|
| `START_MISSION` | `companion_computer` | TCP JSON |
| `STOP_MISSION` | `companion_computer` | TCP JSON |
| `SET_PARAM` | `companion_computer` | TCP JSON |
| `SET_SIMULATOR_COORD_TRANSFORM` | `companion_computer` | TCP JSON (sim only) |
| `PANIC_RTL` | `flight_controller` | MAVLink `MAV_CMD_NAV_RETURN_TO_LAUNCH` (cmd 20) |

`PANIC_RTL` is fixed behavior and MUST map to RTL action regardless of active profile.
It bypasses the JSON envelope entirely and is sent as a raw MAVLink command directly
to the Pixhawk over UDP.

## 7. Command lifecycle semantics (ACK / REJECT / TIMEOUT)

This section applies to **JSON commands sent to the Companion Computer** only.
`PANIC_RTL` is fire-and-forget over MAVLink — see Section 7.5.

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

```json
{
  "status": "ACK",
  "accepted_by": "companion_computer"
}
```

### 7.3 REJECT

Receiver sends `station/commands/reject` when request cannot be accepted.

```json
{
  "status": "REJECT",
  "error_code": "SAFETY_CONSTRAINT",
  "message": "Command blocked by safety policy"
}
```

### 7.4 TIMEOUT

Two timeout classes exist:

- `ACK_TIMEOUT`: no ACK or REJECT received within `T_ack`.
- `EXEC_TIMEOUT`: ACK received, but no completion evidence by `T_exec`.

MVP default timers:

- `T_ack = 2s`
- `T_exec = 10s` (command-specific overrides allowed)

Completion evidence (MVP): deterministic state or event transition proving execution
(e.g., mission enters expected FSM state after `START_MISSION`).

`station/commands/result` is a post-MVP addition. When implemented, it provides
explicit SUCCESS/FAILED completion signaling and structured logging.

### 7.5 `PANIC_RTL` lifecycle

`PANIC_RTL` does not follow the ACK/REJECT/TIMEOUT lifecycle.

- Sent directly to Pixhawk as MAVLink `MAV_CMD_NAV_RETURN_TO_LAUNCH` (cmd 20) over UDP.
- Fire-and-forget: no ACK is expected from Pixhawk at the protocol level.
- Confirmation is implicit: when `vehicle_mode` in the telemetry stream transitions
  to `RTL`, the command is considered delivered and active.
- No automatic background retry. Re-sending `PANIC_RTL` requires explicit operator action.

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
- Receiver MUST deduplicate repeated requests with the same `correlation_id`
  for a minimum 60s window.
- For duplicates, receiver MUST return the same terminal response (`ACK`, `REJECT`,
  or `result`) when available.

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
- `PANIC_RTL` is not subject to this retry policy. Re-sending requires explicit
  operator action only.

## 10. Example message flows

### 10.1 START_MISSION request

```json
{
  "schema_version": "1.0.0",
  "category": "station/commands/request",
  "timestamp": "2026-02-10T19:00:00Z",
  "source": "station",
  "correlation_id": "2cf42dca-d8a2-46d2-bdfd-677ee6a66e8f",
  "payload": {
    "command": "START_MISSION",
    "target": "companion_computer",
    "params": { "mission_id": "mission_1" }
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

### 10.3 STOP_MISSION request

```json
{
  "schema_version": "1.0.0",
  "category": "station/commands/request",
  "timestamp": "2026-02-10T19:01:00Z",
  "source": "station",
  "correlation_id": "9a3f1bcd-0011-4e22-a987-aabbccddeeff",
  "payload": {
    "command": "STOP_MISSION",
    "target": "companion_computer",
    "params": {}
  }
}
```

### 10.4 PANIC_RTL (MAVLink — no JSON envelope)

`PANIC_RTL` bypasses the JSON protocol entirely.

```
MAVLink COMMAND_LONG:
  target_system:    <pixhawk sysid>
  target_component: <pixhawk compid>
  command:          MAV_CMD_NAV_RETURN_TO_LAUNCH (20)
  confirmation:     0
  param1..param7:   0
```

Confirmation is observed via telemetry: `vehicle_mode == "RTL"`.

### 10.5 telemetry/simulator example

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

### 10.6 Runtime transform update command

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

## 11. Simulation mode — ROS2 transport

When `deployment_mode` is `"simulation"`, this JSON protocol does **not** apply to
incoming data. All components connect to rosbridge (rosbridge_suite WebSocket server)
and subscribe to ROS2 topics. The JSON envelope defined in Sections 3–10 is used only
for outgoing commands on the `ros2.topics.commands` topic.

### 11.1 rosbridge connection

```json
"ros2": {
  "bridge": {
    "host": "127.0.0.1",
    "port": 9090
  }
}
```

- Protocol: WebSocket, rosbridge v2.
- On Windows/WSL2: `host` MUST be the WSL2 NIC IP (e.g. `172.x.x.x`), not `127.0.0.1`.

### 11.2 Topic mapping

All topic names are user-configurable in `settings.json` under `ros2.topics`.
The station UI exposes a topic mapping panel in simulation mode for inspecting and
editing topic names. Changes are written to `settings.json` and take effect after
application restart. A **"⚠ Restart required to apply changes"** banner is shown
whenever unapplied topic changes are present.

Default topic mapping:

| Internal channel | Default topic | ROS2 message type |
|-----------------|---------------|-------------------|
| Telemetry | `/mavros/local_position/pose` | `geometry_msgs/PoseStamped` |
| Vehicle state | `/mavros/state` | `mavros_msgs/State` |
| Mission state | `/mission/state` | `std_msgs/String` (JSON payload) |
| Perception output | `/perception/output` | `std_msgs/String` (JSON payload) |
| Safety events | `/safety/events` | `std_msgs/String` (JSON payload) |
| Camera | `/camera/image_raw` | `sensor_msgs/Image` |
| Commands (out) | `/ulak_gcs/commands` | `std_msgs/String` (JSON envelope) |

### 11.3 Data normalization

Each component is responsible for converting ROS2 message types into internal models:

- `geometry_msgs/PoseStamped` → `TelemetryFrame`
- `mavros_msgs/State` → vehicle mode / arm fields of `TelemetryFrame`
- `std_msgs/String` (JSON) → `MissionState` / `PerceptionTarget` / `SafetyEvent`
- `sensor_msgs/Image` → decoded frame buffer for Stream Manager

The Application Core and UI receive the same internal model regardless of deployment mode.

### 11.4 Coordinate transform

In simulation mode the coordinate transform (`simulation.coordinate_transform`) MUST be
enabled to align Gazebo world coordinates with the ArduPilot LOCAL_NED frame before
populating `TelemetryFrame.position_m`.

---

This contract is the MVP baseline. Future versions can extend payload fields and
categories without breaking envelope invariants.