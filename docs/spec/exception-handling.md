# Exception Handling Contract (MVP)

## 1. Purpose

Define how safety/exception events are classified, surfaced, and converted to deterministic station actions.

## 2. Scope

Applies to:

- inbound `safety/events`,
- local validation or lifecycle failures (for example command timeouts),
- profile-driven action mapping for `WARN`, `ERROR`, `CRITICAL`.

## 3. Event contract

Minimum expected event payload:

```json
{
  "severity": "ERROR",
  "code": "LINK_DEGRADED",
  "message": "Telemetry jitter above threshold",
  "recommended_action": "HOLD"
}
```

Rules:

- `severity` MUST be one of `WARN`, `ERROR`, `CRITICAL`.
- `code` MUST be stable, machine-readable, and documented.
- Unknown event codes MUST still be handled via default severity/action mapping.

## 4. Classification model

1. Source event arrives (`safety/events` or internal failure).
2. `ExceptionClassifier` resolves severity and action using active profile mapping.
3. If no explicit mapping exists, default profile fallback mapping is used.
4. Result is published to UI, unified state, and logger.

## 5. Severity behavior contract

### 5.1 WARN

- Non-blocking visibility event.
- No forced command dispatch.
- Timeline and notification must remain visible.

### 5.2 ERROR

- Requires operator decision window.
- Default confirm countdown is 5s (configurable).
- If operator confirms, mapped action is executed.
- If operator cancels, event remains logged and visible.
- If timeout expires without confirmation, mapped safe fallback action executes (default: RTL).

### 5.3 CRITICAL

- Immediate safe action path, no normal confirmation delay.
- CRITICAL visibility must be persistent and unmissable in UI.
- Command dispatch and outcome tracking are mandatory.

## 6. Action mapping

Profile maps each exception code to:

- severity,
- primary action,
- timeout/fallback action (for ERROR),
- optional rate limit/debounce policy.

Actions may include:

- notify only,
- hold/stop mission,
- route-to-home via `PANIC_RTL` contract.

## 7. Protocol and command lifecycle integration

- All command-generating exception actions must use `station/commands/request`.
- `ACK` is not completion; completion requires result or deterministic state evidence.
- `ACK_TIMEOUT` and `EXEC_TIMEOUT` are treated as exception events and re-enter this pipeline.

## 8. Audit and observability requirements

For each exception transition, logger must capture:

- source,
- event code,
- severity,
- selected action,
- profile id,
- correlation id (if command was emitted),
- final lifecycle status.

## 9. Acceptance criteria

- Severity mapping is deterministic across profile switches.
- ERROR countdown behavior supports confirm/cancel/timeout branches.
- CRITICAL events trigger immediate visible and logged response.
- Unknown event codes never crash pipeline and are handled by fallback mapping.

## References

- `docs/adr/002-gcs-profiles.md`
- `docs/adr/003-panic-button-system.md`
- `docs/design/PROTOCOL.md`
- `docs/design/Architecture.md`
