# ULAK GCS — Repository Structure

This document describes the current repository layout and the intended module
boundaries for the ULAK GCS station application. It is aligned with
`docs/design/Architecture.md` and `docs/design/PROTOCOL.md`.

---

## Top-Level Layout

- `assets/`: UI assets such as icons and Qt stylesheets.
- `config/`: Runtime configuration and profile baselines used by the station.
- `docs/`: Design, ADRs, specs, and MVP checklist.
- `scenarios/`: Human-readable mission scenarios and diagrams.
- `src/`: Application source code.
- `tests/`: Unit/smoke tests and test CMake wiring.
- `CMakeLists.txt`: Root build entry.
- `README.md`: Build/run/test instructions.

---

## src/ Layout

- `src/app/`: App entry point and bootstrap utilities.
  - `main.cpp`: CLI parsing and bootstrap flow.
  - `BootstrapConfig.*`: Minimal config validation used by app and tests.
- `src/comms/`: Telemetry, command, and stream adapters (planned).
- `src/core/`: Application core (state aggregation, config management, safety).
- `src/models/`: Data models that mirror protocol contracts.
- `src/ui/`: Qt UI layer and main window definitions.
- `src/utils/`: Common utilities (JSON helpers, etc.).

Note: Some directories are scaffolding for upcoming phases and may be minimal
until the corresponding checklist phase is implemented.

---

## config/ Layout

- `config/settings.json`: Primary station settings (endpoints, stream mode, safety).
- `config/gcs_defaults.json`: Immutable defaults and contract-level settings.
- `config/profiles/`: Policy profiles (`default`, `safe`, `aggressive`).

---

## tests/ Layout

- `tests/CMakeLists.txt`: Test target wiring.
- `tests/test_bootstrap_config.cpp`: Bootstrap/config validation tests.

Tests are executed via `ctest` from the build directory.

---

## Docs Pointers

- `docs/design/Architecture.md`: System boundaries and runtime data flows.
- `docs/design/PROTOCOL.md`: Message envelopes and payload contract.
- `docs/design/checklist.md`: MVP checklist and evidence log.

---

## Build Artifacts

The default CMake configuration writes executables into the build directory
root (for example, `build/sauro_station`) to keep README run commands stable.
