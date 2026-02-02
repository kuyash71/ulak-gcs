# SAÜRO Station

**SAÜRO Station** is a Ground Control Station (GCS) application for the SAÜRO rotary-wing UAV project.
It provides a single operator-friendly interface to **monitor telemetry**, **observe mission/perception outputs**, and **issue controlled commands** to the system.

> Scope note: This repository focuses on the **station / GCS side**. Flight control runs on the flight controller (e.g., Pixhawk/ArduPilot),
> while mission logic and perception may run on a companion computer. The station integrates these data streams into one UI.

---

## Why this exists

In the SAÜRO system, communication is intentionally **hybrid**:

- **Critical flight telemetry** (position, altitude, speed, system status) is expected to reach the station reliably through the flight controller link.
- **Mission state + perception outputs + camera streaming** are expected to be delivered from the companion computer to the station, so the operator can
  observe mission progress and intervene when required. fileciteturn1file8L17-L24

This repo implements the station side of that design.

---

## Features

- 📡 **Telemetry dashboard** (connection status, health, key flight metrics)
- 🧭 **Mission state view** (active FSM state, progress, last transition reason)
- 🔍 **Perception panel** (target info, alignment outputs, confidence levels)
- 🎥 **Camera streaming modes** to balance performance vs observability: fileciteturn1file7L24-L43
  - No stream (max performance)
  - Processed outputs only
  - Compressed live stream (H.264/H.265, FPS/bitrate)
  - (Optional) Full/raw stream for debugging (if enabled)
- 🛡️ **Safety & failsafe visibility** (events, warnings, operator intervention hooks) fileciteturn1file8L4-L9
- 📝 **Structured logging** (session logs, event timeline)

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
│   └── settings.json
├── 📁 docs/                   # Documentation
│   ├── 📁 design/
│   │   ├── Architecture.md    # System & station architecture
│   │   ├── checklist.md
│   │   └── PROTOCOL.md
│   └── 📁 tmp/
│       └── test.txt
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
│   │   └── AppController.h
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
├── LICENSE
└── README.md
```

---

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
.uild\Release\sauro_station.exe
```

---

## Configuration

Configuration files live under `config/` (and/or a user-level config directory depending on platform).
Typical parameters include:

- telemetry endpoints (serial/UDP/TCP)
- mission/perception endpoints
- stream mode (none / outputs / compressed / raw)
- logging directory

See **Architecture** for a clear separation of responsibilities and data flow:

- 📖 `docs/design/Architecture.md`

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

## Contributing

PRs are welcome. Please:

1. Open an issue for major changes
2. Keep commits small and descriptive
3. Add/update docs for user-visible changes
4. Add tests when feasible

---

## License

TBD (choose and add a LICENSE file).

---

## Acknowledgements

This station design follows the SAÜRO software design approach emphasizing a **modular**, **traceable**, and **safety-aware** architecture. fileciteturn1file10L6-L28
