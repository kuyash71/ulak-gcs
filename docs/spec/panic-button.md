# Panic Button Specification (MVP)

## 1. Purpose

Define user-visible and protocol-level behavior of the Panic Button.

## 2. Fixed contract

- Panic action is always `PANIC_RTL`.
- Panic command category is `station/commands/request`.
- Panic command mapping cannot be changed by profile, UI preferences, or runtime customization.

## 3. UX requirements

- Panic control is always visible in main operator workflow.
- Trigger uses a guard pattern (explicit confirmation or protected interaction).
- After trigger, UI shows command lifecycle state: `SENT`, `ACK`, `REJECT`, `TIMEOUT`, `RESULT`.

## 4. Dispatch behavior

1. Generate fresh `correlation_id`.
2. Emit `station/commands/request` with payload:
```json
{
  "command": "PANIC_RTL",
  "target": "flight_controller",
  "params": {}
}
```
3. Start lifecycle timers (`ACK_TIMEOUT`, `EXEC_TIMEOUT`) per protocol defaults or configured overrides.

## 5. Failure behavior

- If ACK is not received before `ACK_TIMEOUT`, create CRITICAL safety event and keep failure visible to operator.
- If ACK is received but no terminal evidence appears before `EXEC_TIMEOUT`, create CRITICAL safety event.
- Re-triggering panic is allowed only by explicit operator action.

## 6. Audit requirements

- Every panic trigger must produce an audit record including:
  - operator identity/session id (if available),
  - timestamp,
  - correlation id,
  - command lifecycle outcome.

## 7. Verification checklist

- Panic control available across all primary UI states.
- Panic always dispatches `PANIC_RTL`.
- Panic cannot be remapped by profile edit attempts.
- Timeout/reject paths produce CRITICAL visibility and audit logs.

## References

- `docs/adr/003-panic-button-system.md`
- `docs/design/PROTOCOL.md`
- `docs/design/Architecture.md`
