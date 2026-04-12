<p align="center">
  <img src="docs/images/ulak-gcs-banner.png" alt="ULAK GCS banner">
</p>

# ULAK GCS

**ULAK GCS** is a Ground Control Station (GCS) application originally developed for the SAÜRO rotary-wing UAV project. After seeing the potential of the station-side architecture, the GCS was separated into this standalone repository.

ULAK GCS is a next-generation GCS designed for ArduPilot-based drone systems with integrated mission visibility, perception observability, and a developer-friendly toolset — while preserving flight-controller authority and safety.

ULAK GCS focuses on an operator-friendly UI to **monitor telemetry**, **observe mission/perception outputs**, and **issue controlled commands**, with a plugin-friendly structure planned to support extensibility.

> **Scope:** This repository covers the **station (GCS) side** only. Flight control runs on the flight controller (e.g., Pixhawk/ArduPilot), while mission logic and perception run on a companion computer (e.g., Raspberry Pi). The station integrates these data streams into one UI.

---

## Why this exists

In the ULAK GCS system, communication is intentionally **hybrid**:

- **Critical flight telemetry** (position, altitude, speed, system status) reaches the station through the MAVLink link over WiFi telemetry.
- **Camera stream** is delivered from the companion computer to the station over a separate channel, so the operator can observe mission progress and intervene when required.

The GCS acts as a **monitor and intervention point** — all autonomous decisions are made locally on the vehicle. The station observes, logs, and provides the operator with controlled command hooks.

This repo implements the station side of that design.

---

## Features

- 📡 **Telemetry dashboard** (connection status, health, key flight metrics)
- 🧭 **Mission state view** (active FSM state, progress, last transition reason)
- 🔍 **Perception panel** (target info, alignment outputs, confidence levels)
- 🎥 **Camera streaming modes** to balance performance vs observability:
  - No stream (max performance)
  - Processed outputs only
  - Compressed live stream (H.264/H.265)
  - (Optional) Full/raw stream for debugging (if enabled)
- 🛡️ **Safety & failsafe visibility** (events, warnings, operator intervention hooks)
- 📝 **Structured logging** (session logs, event timeline)
- 🔧 **Polisher** (dev tool, requires camera mode active)
  - Load a compatible `.py` vision script via file picker
  - Dynamically tune `@polisher_param` annotated values through an interactive panel
  - Preview changes live on the active camera feed
- 🧩 **Plugin-friendly architecture** (Lua + JSON, WIP)
  - Lua is planned for runtime plugin logic
  - JSON is used for declarative configuration and policy enforcement

---

## Modes

ULAK GCS operates in two independent modes, each with its own entry point under `src/platforms/`:

**SIM Mode** — connects to an ArduPilot SITL instance and a Gazebo ROS2 pipeline. Telemetry is received via MAVLink over UDP. Coordinate transforms (Gazebo-to-ArduPilot) are configurable.

**DRONE Mode** — connects to a Pixhawk (2.4.8 or Orange Cube) over WiFi telemetry for MAVLink data, and to a Raspberry Pi 4B over a separate channel for the camera stream.

Both modes share the same core, UI, and configuration system.

---

## Design Decisions

- GCS is a monitor and operator intervention point — autonomous decisions stay on the vehicle
- Customizable GCS behaviour via Profile System
- Panic Button → RTL (fixed, cannot be changed via profiles or customization)
- Exception levels: WARN / ERROR / CRITICAL (3 levels, fully customizable per profile)
- Policy-first approach throughout the project
- Canonical telemetry contract is XYZ-based for both vehicle and simulator paths
- Telemetry variants are named `telemetry/vehicle` and `telemetry/simulator`
- Simulation telemetry supports configurable Gazebo-to-ArduPilot coordinate transform
- Default profile is non-removable via GUI
- Panic Button behavior is independent of active profile
- Polisher panel is only accessible when camera mode is active
- Polisher scripts are copied to local storage on load; the original file is not modified

## Non-Goals

- Not a full QGC replacement
- Not a cloud or multi-user panel
- No manual flight control

---

## Repository Layout

```
.
├── src/
│   ├── core/              # AppController, ProfileManager, PanicManager, ExceptionClassifier
│   ├── comms/             # MAVLink client, video stream client, command client
│   ├── models/            # TelemetryFrame, MissionState, PerceptionTarget, GcsCommand, StreamMode
│   ├── ui/                # MainWindow, panels, widgets
│   ├── utils/             # Shared utilities (JSON, logging, etc.)
│   └── platforms/
│       ├── sim/
│       │   └── main.py    # SIM mode entry point
│       └── drone/
│           └── main.py    # DRONE mode entry point
├── config/
│   ├── profiles/
│   │   ├── default.json
│   │   ├── safe.json
│   │   ├── aggressive.json
│   │   └── README.md
│   ├── gcs_defaults.json
│   └── settings.json
├── assets/
│   ├── icons/
│   └── themes/
├── docs/
│   ├── adr/               # Architecture Decision Records
│   ├── design/            # Architecture, protocol, checklist
│   ├── images/
│   └── spec/              # Ecosystem, exception handling, panic button, polisher specs
├── tests/
├── build/                 # Build outputs (platform installers, frozen binaries)
├── .clang-format          # Kept for reference; not active
├── .gitignore
├── LICENSE
└── README.md
```

---

## Polisher

Polisher is a developer tool built into the GCS camera panel. It allows runtime tuning of vision pipeline parameters without restarting the application.

**How it works:**

1. Enable camera mode
2. Open the Polisher switch inside the camera panel
3. Select a compatible `.py` vision script via file picker — it will be copied to local storage
4. Any parameters declared within the `@polisher_start` / `@polisher_end` block and annotated with `@polisher_param` will appear as interactive controls in the Polisher panel
5. Adjusting a control updates the parameter value live on the active feed

**Writing a Polisher-compatible script:**

```python
@polisher_start

@polisher_param(label="Threshold", type="slider", min=0, max=255)
threshold = 127

@polisher_param(label="Blur Kernel", type="slider", min=1, max=21, step=2)
blur_kernel = 5

@polisher_param(label="Edge Detection", type="toggle")
use_edge = True

@polisher_end
```

Supported `type` values (MVP): `slider`, `toggle`. The type system is extensible.

---

## Prerequisites

- Python 3.10+
- PyQt6
- pymavlink
- OpenCV (`opencv-python`)
- GStreamer (optional, for compressed stream modes)

---

## Configuration

Configuration files live under `config/`. Typical parameters include:

- Telemetry endpoints (UDP/TCP/serial)
- Simulation coordinate transform (source frame, target frame, axis mapping, offsets)
- Mission/perception endpoints
- Stream mode (`none` / `outputs` / `compressed` / `raw`)
- Logging directory
- Active profile

See `docs/design/Architecture.md` for a full breakdown of responsibilities and data flow.

---

## Development Workflow

### Recommended tools

- VS Code with Pylance + Python extension
- Qt Designer (for `.ui` files if used alongside PyQt6)

### Style & quality

- Keep the UI thread responsive — MAVLink polling and video decoding must run in worker threads
- Prefer clear interfaces between layers (`comms` → `core` → `ui`)
- Add tests for parsing, state transitions, and safety-critical behavior (Panic, exception levels)
- Polisher-compatible scripts must declare all tunable parameters inside the `@polisher_start` / `@polisher_end` block

---

## Roadmap

- [ ] Persistent mission timeline panel (filterable)
- [ ] Replay mode (load session logs / recorded streams)
- [ ] Operator checklists (pre-flight, in-flight, landing)
- [ ] Lua plugin system (WIP)
- [ ] Build pipeline (platform installers via PyInstaller or equivalent)

---

## Screenshots

Screenshots will be added when the first UI milestone is reached.

---

## Contributing

PRs are welcome. Please:

1. Open an issue for major changes
2. Keep commits small and descriptive
3. Add or update docs for user-visible changes
4. Add tests when feasible

---

## License

Apache License, Version 2.0. See `LICENSE` for details.

---

## Acknowledgements

This station design follows the ULAK software design approach emphasizing a **modular**, **traceable**, and **safety-aware** architecture.