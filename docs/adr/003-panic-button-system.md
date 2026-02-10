# ADR-003: Panic Button Is Fixed to RTL

- Status: Accepted
- Date: 2026-02-10
- Owners: ULAK GCS maintainers

## Context

In high-stress conditions, operator response must be immediate and deterministic. Allowing profile-specific panic behavior creates ambiguity and increases operational risk. MVP therefore needs a single globally consistent panic action.

## Decision

1. Panic command is fixed.
- Panic always emits `PANIC_RTL`.
- Semantic outcome is Return-to-Launch (RTL) request.

2. Panic command is non-customizable.
- Profiles cannot remap panic to another action.
- UI settings cannot remap panic.
- Runtime commands cannot redefine panic behavior.

3. Panic is globally available in operator UX.
- Panic control is always visible in the main operator flow.
- Trigger requires guarded confirmation according to UI safety policy.

4. Panic is fully auditable.
- The following chain is logged: trigger intent, command dispatch, transport result (ACK/REJECT/TIMEOUT), and terminal outcome when available.

5. Failure handling is explicit.
- If endpoint is unreachable or command times out, station raises a CRITICAL safety notification and logs the failure with correlation id.

## Consequences

- Operator training and emergency behavior become predictable.
- Multi-team deployments share one emergency rule set.
- Flexibility is intentionally reduced for panic behavior.

## References

- `docs/design/Architecture.md`
- `docs/design/PROTOCOL.md`
- `docs/spec/panic-button.md`
- `docs/spec/exception-handling.md`
