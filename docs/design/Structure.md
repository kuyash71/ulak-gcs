# ULAK GCS — Repository Structure

This document describes the current repository layout and the intended module
boundaries for the ULAK GCS station application. It is aligned with
`docs/design/Architecture.md` and `docs/design/PROTOCOL.md`.

---

## Top-Level Layout

- `assets/`: UI assets such as icons and Qt stylesheets.
- `config/`: Runtime configuration and profile baselines used by the station.
- `docs/`: Design documents, ADRs, specs, and MVP checklist.
- `src/`: Application source code.
- `tests/`: Unit and smoke tests.
- `build/`: Build outputs (platform installers, frozen binaries). Not tracked by version control.
- `README.md`: Project overview, prerequisites, and setup instructions.

---

## src/ Layout

```
src/
├── core/           # Application core: state aggregation, config management, safety
├── comms/          # Telemetry, command, and stream components
├── models/         # Data models mirroring protocol contracts
├── ui/             # PyQt6 UI layer: main window, panels, widgets
├── utils/          # Shared utilities (JSON helpers, logging, etc.)
└── platforms/      # Deployment-specific entry points
    ├── sim/
    │   └── main.py     # SIM mode entry point (SITL + Gazebo / ROS2)
    └── drone/
        └── main.py     # DRONE mode entry point (Pixhawk WiFi + Raspberry Pi)
```

### src/core/

Application state aggregation and safety-critical logic:

- `AppController`: central coordinator, owns unified system state.
- `ProfileManager`: loads and applies policy profiles.
- `PanicManager`: handles `PANIC_RTL` dispatch to Pixhawk via MAVLink.
- `ExceptionClassifier`: classifies incoming safety events into WARN / ERROR / CRITICAL.
- `ConfigLoader`: reads and validates `settings.json` and `gcs_defaults.json`.

### src/comms/

Transport components, one per data channel:

- `TelemetryComponent`: receives and normalizes telemetry from FC (MAVLink/UDP in drone mode, rosbridge in sim mode).
- `MissionComponent`: receives mission state, perception output, and safety events from CC (TCP JSON or rosbridge).
- `VideoStreamComponent`: manages camera frame delivery to the UI (H.264 or rosbridge).
- `CommandGateway`: validates and dispatches operator commands to the correct target and transport.

### src/models/

Typed data models that mirror the protocol contracts defined in `docs/design/PROTOCOL.md`:

- `TelemetryFrame`: normalized vehicle/simulator telemetry.
- `MissionState`: active FSM state, progress, transition note.
- `PerceptionTarget`: target info, alignment, confidence.
- `SafetyEvent`: severity, code, message, recommended action.
- `GcsCommand`: validated operator command with routing metadata.
- `StreamMode`: stream mode enum (`OFF`, `OUTPUTS_ONLY`, `COMPRESSED_LIVE`, `RAW_DEBUG`).

### src/ui/

PyQt6 UI layer:

- `MainWindow`: top-level window, mode selector, global layout.
- Panels: telemetry dashboard, mission state view, perception panel, camera/stream panel, safety timeline.
- `PolisherPanel`: developer tool panel, accessible only when camera mode is active. See `docs/spec/polisher.md`.
- `TopicMappingPanel`: ROS2 topic editor, visible in simulation mode only. Writes to `settings.json`, requires restart to apply.

### src/utils/

Shared utilities with no dependency on core or UI:

- JSON parsing and validation helpers.
- Structured logger.
- General-purpose utilities.

### src/platforms/

Each entry point bootstraps the full application stack for its deployment mode:

- `sim/main.py`: configures all components for simulation mode (rosbridge transport, coordinate transform).
- `drone/main.py`: configures all components for real drone mode (MAVLink/UDP + TCP JSON transports).

Both entry points share the same `core`, `comms`, `models`, `ui`, and `utils` modules.

---

## config/ Layout

```
config/
├── settings.json           # Primary station settings (endpoints, stream mode, deployment mode)
├── gcs_defaults.json       # Immutable defaults and contract-level settings
├── profiles/
│   ├── default.json        # Default profile (non-removable via GUI)
│   ├── safe.json           # Conservative intervention thresholds
│   ├── aggressive.json     # Permissive thresholds, more operator responsibility
│   └── README.md           # Profile authoring guide
└── polisher/
    └── scripts/            # Loaded Polisher vision scripts
```

---

## tests/ Layout

```
tests/
├── test_telemetry_parse.py     # TelemetryFrame parsing and normalization
├── test_command_serialize.py   # GcsCommand validation and routing
├── test_exception_classifier.py # WARN / ERROR / CRITICAL classification
└── test_panic_manager.py       # PANIC_RTL dispatch behavior
```

Tests are run with:

```bash
pytest tests/
```

---

## assets/ Layout

```
assets/
├── icons/      # SVG and PNG icons used in the UI
└── themes/     # Qt stylesheets (.qss)
```

---

## docs/ Layout

```
docs/
├── adr/                        # Architecture Decision Records
│   ├── 001-customizable-gcs-vision.md
│   ├── 002-gcs-profiles.md
│   ├── 003-panic-button-system.md
│   └── ...
├── design/
│   ├── Architecture.md         # System boundaries and runtime data flows
│   ├── PROTOCOL.md             # Message envelopes and payload contracts
│   ├── checklist.md            # MVP checklist and evidence log
│   └── structure.md            # This document
├── images/
│   └── ulak-gcs-banner.png
└── spec/
    ├── ecosystem.md            # What is and is not customizable
    ├── exception-handling.md   # WARN / ERROR / CRITICAL definitions and profile mapping
    ├── panic-button.md         # Panic Button contract and constraints
    └── polisher.md             # Polisher tool specification
```

---

## build/ Layout

Build outputs are written to `build/` and are not tracked by version control.
This directory is intended for platform-specific frozen binaries produced by
PyInstaller or an equivalent tool in a future build pipeline phase.

---

> Note: Some `src/` subdirectories may be minimal stubs until the corresponding
> MVP checklist phase is implemented.