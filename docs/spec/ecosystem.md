# GCS Customization Ecosystem Boundaries (MVP)

## 1. Purpose

Define what can be customized and what is intentionally fixed in ULAK GCS MVP.

## 2. Customizable areas

- Profile mappings (`event_code -> severity -> action`)
- Endpoint and transport settings
- Stream mode preferences
- Simulation coordinate transform settings
- UI presentation details that do not change safety invariants

## 3. Non-customizable safety invariants

- Panic behavior is fixed to `PANIC_RTL`
- Severity model is fixed to `WARN`, `ERROR`, `CRITICAL`
- Command lifecycle semantics remain protocol-defined (`ACK`, `REJECT`, `TIMEOUT`)
- Critical audit chain requirements cannot be disabled

## 4. Extension model

Future extensions should preserve protocol compatibility by:

- keeping envelope fields stable,
- versioning schema changes,
- avoiding silent behavior changes for existing event codes.

## 5. Governance

- Every new customization point requires:
  - explicit documentation,
  - rollback strategy,
  - test evidence for safety regressions.

## References

- `docs/design/PROTOCOL.md`
- `docs/design/Architecture.md`
- `docs/adr/002-gcs-profiles.md`
- `docs/adr/003-panic-button-system.md`
