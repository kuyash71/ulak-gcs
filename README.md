<p align="center">
  <img src="docs/images/ulak-gcs-banner.png" alt="ULAK GCS banner">
</p>

# ULAK GCS

**ULAK GCS** is a Ground Control Station (GCS) application originally developed for the SAÜRO rotary-wing UAV project.
After seeing the potential of the station-side architecture, the GCS was separated into this standalone repository to
serve a broader **ArduPilot**-oriented ecosystem.

ULAK GCS focuses on an operator-friendly UI to **monitor telemetry**, **observe mission/perception outputs**, and **issue controlled commands**,
with a plugin-friendly structure planned to support extensibility.

> **Scope:** This repository covers the **station (GCS) side** only. Flight control runs on the flight controller (e.g., Pixhawk/ArduPilot),
> while mission logic and perception may run on a companion computer. The station integrates these data streams into one UI.

---

## Why this exists

In the ULAK GCS system, communication is intentionally **hybrid**:

- **Critical flight telemetry** (position, altitude, speed, system status) is expected to reach the station reliably through the flight controller link.
- **Mission state + perception outputs + camera streaming** are expected to be delivered from the companion computer to the station, so the operator can
  observe mission progress and intervene when required.

This repo implements the station side of that design.

---

## Features

- 📡 **Telemetry dashboard** (connection status, health, key flight metrics)
- 🧭 **Mission state view** (active FSM state, progress, last transition reason)
- 🔍 **Perception panel** (target info, alignment outputs, confidence levels)
- 🎥 **Camera streaming modes** to balance performance vs observability:
  - No stream (max performance)
  - Processed outputs only
  - Compressed live stream (H.264/H.265, FPS/bitrate)
  - (Optional) Full/raw stream for debugging (if enabled)
- 🛡️ **Safety & failsafe visibility** (events, warnings, operator intervention hooks)
- 📝 **Structured logging** (session logs, event timeline)
- 🧩 Plugin-friendly architecture (Lua + JSON, WIP)
  - Lua is used for runtime plugin logic
  - JSON is used for declarative configuration and policy enforcement

---

## Repository layout

> The exact internal structure may evolve; these are the intended boundaries.

```
.
├── 📁 assets/                 # Icons, images, UI assets
│   ├── 📁 icons/
│   │   └── example.svg
│   └── 📁 themes/
│       └── dark.qss
├── 📁 config/                 # App configuration (ports, endpoints, UI presets)
│   ├── 📁 profiles/
│   │   ├── aggressive.json # Aggressive profile for more user responsibility and less system intervention
│   │   ├── default.json # Default profile
│   │   ├── safe.json # Safe profile for less user responsibility and more system intervention
│   │   └── README.md # README about how to customize your own profile
│   ├── gcs_defaults.json # Default of GCS Sim
│   └── settings.json # Settings of GCS
├── 📁 docs/
|   ├── 📁 adr/
│   │   ├── 001-customizable-gcs-vision.md    # Customizable GCS behaviour
│   │   ├── 002-gcs-profiles.md # GCS Profiles and matching the Profile specs with w/e/c messages
│   │   ├── 003-panic-button-system.md # Panic Button --> Triggers Status: RTL
│   │   └── ...
│   ├── 📁 design/
│   │   ├── Architecture.md    # System & station architecture
│   │   ├── checklist.md
│   │   └── PROTOCOL.md
|   ├── 📁 images/
│   │   ├── ulak-gcs-banner.png  # Banner Of the Project
│   │   └── ...
│   └── 📁 spec/
│       ├── ecosystem.md # Answers the question: Which parts of the GCS can be customizable?
│       ├── exception-handling.md # Definitions of w/e/c messages, default actions and profile system
│       ├── panic-button.md # When you can "Panic" and what "Panic" does?
│       └── polisher.md # TBD
├── 📁 src/                    # Application source code
│   ├── 📁 app/
│   │   ├── CMakeLists.txt
│   │   └── main.cpp
│   ├── 📁 comms/
│   │   ├── CommandClient.cpp
│   │   ├── CommandClient.h
│   │   ├── TelemetryClient.cpp
│   │   ├── TelemetryClient.h
│   │   ├── VideoStreamClient.cpp
│   │   └── VideoStreamClient.h
│   ├── 📁 core/
│   │   ├── AppConfig.cpp
│   │   ├── AppConfig.h
│   │   ├── AppController.cpp
│   │   ├── AppController.h
│   │   ├── ConfigLoader.cpp
│   │   ├── ExceptionClassifier.cpp
│   │   ├── ExceptionClassifier.h
│   │   ├── PanicManager.cpp
│   │   ├── PanicManager.h
│   │   ├── ProfileManager.cpp
│   │   └── ProfileManager.h
│   ├── 📁 models/
│   │   ├── GcsCommand.h
│   │   ├── MissionState.h
│   │   ├── PerceptionTarget.h
│   │   ├── StreamMode.h
│   │   └── TelemetryFrame.h
│   ├── 📁 ui/
│   │   ├── MainWindow.cpp
│   │   ├── MainWindow.h
│   │   └── MainWindow.ui
│   └── 📁 utils/
│       ├── JsonUtils.cpp
│       └── JsonUtils.h
├── 📁 tests/                  # Unit/integration tests
│   ├── CMakeLists.txt
│   ├── test_command_serialize.cpp
│   └── test_telemetry_parse.cpp
├── .clang-format
├── .gitignore
├── CMakeLists.txt             # CMake build entry
├── LICENSE # Apache License Version 2.0
└── README.md
```

---

## Design Decisions

- Customizable GCS (Profile System and Behaviour Configuration)
- Panic Button => RTL (Cannot be Changed via Customization)
- Exception Levels: WARN / ERROR / CRITICAL (Levels of 3, fully customizable)
- Policy-First approach on project
- Canonical telemetry contract is XYZ-based for both vehicle and simulator paths
- Telemetry variants are named `telemetry/vehicle` and `telemetry/simulator`
- Simulation telemetry supports configurable Gazebo-to-ArduPilot coordinate transform
- Default Profile is non-removable via GUI
- Panic Button behavior is fixed to RTL and independent of profiles
- Not tightly coupled to the ArduPilot UI ecosystem

## Non-Goals

- Not a Full QGC Replacement
- Not a cloud multi-user panel
- No manual flight control

## Prerequisites

- **CMake** 3.20+
- A C++ compiler supporting **C++17**
- **Qt 6** (recommended) or Qt 5 (if the project is configured for it)
- (Optional) **GStreamer / FFmpeg** if video streaming is enabled via native pipelines
- (Optional) A MAVLink endpoint (ArduPilot SITL or real vehicle) for live telemetry testing

---

## Build

### Linux / macOS

```bash
# from repository root
mkdir -p build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

### Windows (Visual Studio)

```powershell
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

> If Qt is not auto-detected, set `CMAKE_PREFIX_PATH` to your Qt installation.

---

## Run

```bash
./build/sauro_station
```

or on Windows:

```powershell
.build\Release\sauro_station.exe
```

---

## Configuration

Configuration files live under `config/` (and/or a user-level config directory depending on platform).
Typical parameters include:

- telemetry endpoints (serial/UDP/TCP)
- simulation coordinate transform (source frame, target frame, axis mapping, offsets)
- mission/perception endpoints
- stream mode (none / outputs / compressed / raw)
- logging directory

See **Architecture** for a clear separation of responsibilities and data flow:

- 📖 `docs/design/Architecture.md`

---

## Polisher (WIP)

- Parameters defined via JSON
- Plugin system planned via Lua
- Final API will be designed after GUI stabilization

---

## Development workflow

### Recommended tools

- Qt Creator (best for Qt UI workflows)
- CLion / VS Code (CMake-based workflows)

### Style & quality

- Keep UI thread responsive (heavy IO/decoding must be offloaded to worker threads)
- Prefer clear interfaces (`ITelemetrySource`, `IStreamSource`, `ICommandBus`)
- Add tests for parsing, state transitions, and safety behavior

---

## Roadmap (WIP)

- [ ] Persistent mission timeline panel (filterable)
- [ ] Replay mode (load logs / recorded streams)
- [ ] Operator checklists (pre-flight, in-flight, landing)
- [ ] Multi-vehicle support (profiles)

---

## Screenshots

Screenshots will be added on this section when available

## Contributing

PRs are welcome. Please:

1. Open an issue for major changes
2. Keep commits small and descriptive
3. Add/update docs for user-visible changes
4. Add tests when feasible

---

## License

Apache License, Version 2.0.
See `LICENSE` for details.

---

## Acknowledgements

This station design follows the ULAK software design approach emphasizing a **modular**, **traceable**, and **safety-aware** architecture.
