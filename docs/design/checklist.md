# SAURO Station - MVP Completion Checklist

This checklist is the MVP definition of done for this repository.
When every item in this file is checked with evidence, no MVP-scoped work should remain.

Primary references:

- `README.md`
- `docs/design/Architecture.md`
- `docs/design/PROTOCOL.md`

Core decisions this checklist enforces:

- Policy-first behavior (profiles control severity/actions)
- Panic Button is fixed to RTL (never profile-overridable)
- Three exception levels: WARN / ERROR / CRITICAL
- Hybrid communication model (FC telemetry + CC mission/perception/stream)

---

## How To Use This Checklist

- `Build`: durable product artifact (code/config/UI/doc contract)
- `Test`: automated or reproducible manual verification
- `Docs`: written contract/runbook/evidence requirement
- `Gate`: hard exit criterion; MVP is blocked until complete

Rules:

- Every checked item must have evidence in the `Evidence Log` section.
- `N/A` is allowed only with a short rationale in evidence.
- No item under Phases 0-8 is optional for MVP.

---

## Phase 0 - Scope Lock And Consistency

- [x] **Gate**: `README.md`, `docs/design/Architecture.md`, and `docs/design/PROTOCOL.md` describe the same MVP boundaries and command/safety model.
- [x] **Docs**: `docs/design/PROTOCOL.md` is no longer empty and defines:
  - [x] message categories (`telemetry/*`, `mission/state`, `perception/output`, `safety/events`, `station/commands`)
  - [x] envelope fields (`schema_version`, `timestamp`, `source`, `correlation_id`)
  - [x] command ACK/REJECT/TIMEOUT semantics
  - [x] error codes and retry/idempotency rules
- [x] **Docs**: ADR/spec files are populated (not placeholders) for:
  - [x] profiles and policy mapping
  - [x] panic button fixed behavior
  - [x] exception handling contract
- [x] **Build**: config baselines exist and contain valid JSON content:
  - [x] `config/settings.json`
  - [x] `config/gcs_defaults.json`
  - [x] `config/profiles/default.json`
  - [x] `config/profiles/safe.json`
  - [x] `config/profiles/aggressive.json`

---

## Phase 1 - Build System And App Bootstrap

- [ ] **Build**: `CMakeLists.txt` defines project, app target, and test target(s).
- [ ] **Build**: `src/app/main.cpp` boots app, handles invalid config, and exits cleanly.
- [ ] **Build**: minimum executable is produced (`sauro_station` or platform equivalent).
- [ ] **Test**: clean-clone configure/build works:
  - [ ] `cmake -S . -B build`
  - [ ] `cmake --build build`
- [ ] **Test**: app start smoke test documented (startup + clean shutdown).
- [ ] **Gate**: build/test instructions in `README.md` run exactly as written.

---

## Phase 2 - Core Contracts: Config, Models, Serialization

- [ ] **Build**: `AppConfig` and `ConfigLoader` load and validate all required files.
- [ ] **Build**: `JsonUtils` provides parse/validate/error-reporting utilities used by core modules.
- [ ] **Build**: models are defined with stable fields and version awareness:
  - [ ] `TelemetryFrame`
  - [ ] `MissionState`
  - [ ] `PerceptionTarget`
  - [ ] `GcsCommand`
  - [ ] `StreamMode`
- [ ] **Build**: command serialization/parsing is deterministic and protocol-compliant.
- [ ] **Test**: config parse tests cover missing file, invalid JSON, invalid schema/value.
- [ ] **Test**: command serialization tests cover malformed/missing/extra fields.
- [ ] **Gate**: config and profile schema_version mismatches result in deterministic fallback behavior as documented (no silent acceptance).

---

## Phase 3 - Policy-First Safety Core

- [ ] **Build**: `ProfileManager` loads default/safe/aggressive profiles and supports switching.
- [ ] **Build**: default profile cannot be deleted from UI workflow.
- [ ] **Build**: `ExceptionClassifier` maps `event -> severity -> action` deterministically.
- [ ] **Build**: ERROR confirm window implemented (default countdown 5s, configurable).
- [ ] **Build**: timeout path for unconfirmed ERROR escalates to configured safe action (RTL by default).
- [ ] **Build**: `PanicManager` always emits RTL command, independent of active profile.
- [ ] **Build**: panic/event safety actions are always audit-logged.
- [ ] **Test**: panic is RTL under all profiles.
- [ ] **Test**: countdown behavior (5 to 0), cancel/confirm/timeout branches.
- [ ] **Gate**: policy behavior matches documented profile mappings exactly.

---

## Phase 4 - Communications And Command Gateway

- [ ] **Build**: `TelemetryClient` supports configured FC transport(s) and connection states.
- [ ] **Build**: CC adapter path consumes mission state, perception output, and safety events.
- [ ] **Build**: `VideoStreamClient` + stream mode negotiation implemented:
  - [ ] OFF
  - [ ] OUTPUTS_ONLY
  - [ ] COMPRESSED_LIVE
  - [ ] RAW_DEBUG (if enabled)
- [ ] **Build**: Command Gateway backend exists (not UI-only) and performs:
  - [ ] schema validation
  - [ ] state/safety gating
  - [ ] endpoint routing (FC vs CC)
  - [ ] correlation-id tracking
  - [ ] audit event emission
- [ ] **Test**: telemetry parser handles malformed/partial/corrupt payloads.
- [ ] **Test**: link drop/reconnect/backoff behavior is verified.
- [ ] **Test**: command lifecycle verified (queued/sent/ack/rejected/timeout).
- [ ] **Gate**: ACK is not treated as completion unless completion event arrives.

---

## Phase 5 - Unified State, Event Bus, Logging

- [ ] **Build**: event bus implemented and used by adapters/core/UI boundary.
- [ ] **Build**: unified state is the single source of truth for:
  - [ ] connection states
  - [ ] telemetry snapshot
  - [ ] mission snapshot/history
  - [ ] perception snapshot
  - [ ] active safety events/timeline
  - [ ] command lifecycle states
- [ ] **Build**: logger writes structured NDJSON events with versioned schema.
- [ ] **Build**: session metadata includes start/end time, profile, config fingerprint.
- [ ] **Test**: event ordering and timestamps are monotonic/consistent.
- [ ] **Test**: log durability on clean shutdown and controlled crash path is validated.
- [ ] **Gate**: each critical operator action has a traceable log chain.

---

## Phase 6 - GUI MVP Surfaces (Qt)

- [ ] **Build**: main window shell with stable regions (status, workspace, logs dock, panic area).
- [ ] **Build**: always-visible global status shows FC link, CC link, active profile, highest severity.
- [ ] **Build**: telemetry panel shows key flight metrics + freshness indicator.
- [ ] **Build**: mission panel shows current FSM state, progress, last transition reason, history.
- [ ] **Build**: perception panel shows target/confidence/alignment + last update age.
- [ ] **Build**: stream panel supports mode switch and stream health indicators.
- [ ] **Build**: safety center shows notifications + filterable timeline.
- [ ] **Build**: controlled commands panel shows full command lifecycle and ERROR countdown UX.
- [ ] **Build**: settings/profile screen supports endpoint edits + profile switching + validation feedback.
- [ ] **Build**: panic UX contract implemented (always visible, guarded trigger, post-panic lockouts).
- [ ] **Build**: logs panel accessible from all views with severity/source/search filters.
- [ ] **Test**: UI remains responsive under update load (telemetry + stream + events).
- [ ] **Test**: repeated start/stop cycles complete without thread leaks or deadlocks.
- [ ] **Gate**: UI acts as renderer/command surface only; business logic stays in core.
- [ ] **Gate**: UI remains responsive under nominal load (no frame freeze > 200ms on target system).

---

## Phase 7 - End-To-End Validation

- [ ] **Test**: nominal mission flow scenario passes:
  - [ ] connect FC+CC
  - [ ] observe telemetry/mission/perception updates
  - [ ] send controlled commands
  - [ ] complete or stop mission with expected logs
- [ ] **Test**: fault scenarios pass with expected profile behavior:
  - [ ] telemetry loss
  - [ ] CC link loss
  - [ ] stream loss
  - [ ] corrupt payload
  - [ ] missing perception updates
  - [ ] **Test**: invalid operator actions (out-of-order commands, rapid toggles, repeated panic attempts) are rejected safely and audit-logged.
- [ ] **Test**: panic can be triggered from all relevant UI states and always dispatches RTL once.
- [ ] **Test**: profile switch changes severity/action behavior deterministically.
- [ ] **Test**: persistence checks:
  - [ ] last selected profile
  - [ ] last stream mode
  - [ ] panel/window layout
- [ ] **Gate**: all test scenarios have reproducible steps and pass criteria documented.

---

## Phase 8 - MVP Release Gate

- [ ] **Docs**: `README.md` contains accurate quickstart for build, run, config, and test.
- [ ] **Docs**: known limitations are explicitly listed (only true MVP limitations).
- [ ] **Docs**: operator runbook exists for:
  - [ ] startup and connection checks
  - [ ] command usage rules
  - [ ] panic usage and expected outcome
  - [ ] incident/failsafe handling
- [ ] **Build**: release artifact layout is defined and reproducible for target platform.
- [ ] **Gate**: a fresh environment can install, configure, run, and verify core scenario using docs only.
- [ ] **Gate**: product owner/team sign-off recorded with date + commit hash.

---

## MVP Exit Criteria (All Must Be True)

- [ ] Policy + profiles + exception pipeline work end-to-end.
- [ ] Panic is always RTL and cannot be overridden by profile/config/UI.
- [ ] FC telemetry + CC mission/perception/stream reach unified state and UI.
- [ ] Command Gateway enforces validation/gating/routing/audit.
- [ ] Safety events are visible, actionable, and fully logged.
- [ ] GUI provides telemetry, mission, perception, stream, safety, logs, commands, and profile settings.
- [ ] Build/test/run/docs are reproducible from a clean clone.
- [ ] No unresolved MVP-scoped TODO remains in code/docs/checklist.

---

## Out Of Scope For MVP (Track Separately)

These items must not block MVP closure and should live in a post-MVP backlog:

- multi-vehicle support
- replay mode with recorded streams
- plugin/Lua polisher ecosystem
- advanced packaging variants beyond primary target platform
- optional visual polish not affecting safety/operability

---

## Evidence Log

Record proof for each completed phase:

- 2026-02-10 protocol update: `docs/design/PROTOCOL.md` populated with MVP protocol contract.
- 2026-02-10 consistency update: telemetry model aligned across `README.md`, `docs/design/Architecture.md`, and `docs/design/PROTOCOL.md` (XYZ base schema + vehicle/simulator variants + configurable simulator coordinate transform).
- 2026-02-10 ADR/spec update:
  - `docs/adr/002-gcs-profiles.md` populated (profiles and policy mapping decision).
  - `docs/adr/003-panic-button-system.md` populated (panic fixed RTL decision).
  - `docs/spec/exception-handling.md` populated (exception handling contract).
  - `docs/spec/panic-button.md` and `docs/spec/ecosystem.md` populated for operational and customization boundaries.
- 2026-02-10 config baseline update:
  - `config/settings.json`, `config/gcs_defaults.json`, `config/profiles/default.json`, `config/profiles/safe.json`, `config/profiles/aggressive.json` populated and validated via `jq`.
- Audit basis:
  - Root `CMakeLists.txt`, `src/`, and `tests/` implementation files are empty placeholders.
- Build commands and outputs:
- Test suite outputs:
- End-to-end scenario evidence:
- Example NDJSON log snippets:
- UI screenshots or short recordings:
- Sign-off (name, date, commit hash):
