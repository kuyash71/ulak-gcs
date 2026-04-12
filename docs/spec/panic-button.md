# Panic Button Specification (MVP)

## 1. Purpose

Define user-visible and protocol-level behavior of the Panic Button.

## 2. Fixed contract

- Panic action is always `PANIC_RTL`.
- Panic is dispatched directly to the Pixhawk as a MAVLink command over UDP.
- Panic does **not** use the JSON protocol envelope (`station/commands/request`).
- Panic command behavior cannot be changed by profile, UI preferences, or runtime customization.

## 3. UX requirements

- Panic control is always visible in main operator workflow.
- Trigger uses a guard pattern (explicit confirmation or protected interaction).
- After trigger, UI shows panic state: `SENT`, `CONFIRMED`, `FAILED`.
  - `SENT`: MAVLink command has been dispatched.
  - `CONFIRMED`: telemetry stream shows `vehicle_mode == "RTL"`.
  - `FAILED`: confirmation window elapsed without `vehicle_mode` transitioning to `RTL`.

## 4. Dispatch behavior

1. Operator triggers Panic Button (guard pattern satisfied).
2. `PanicManager` dispatches MAVLink `COMMAND_LONG` directly to Pixhawk over UDP:

```
MAVLink COMMAND_LONG:
  target_system:    <pixhawk sysid>
  target_component: <pixhawk compid>
  command:          MAV_CMD_NAV_RETURN_TO_LAUNCH (20)
  confirmation:     0
  param1..param7:   0
```

3. UI transitions to `SENT` state immediately.
4. Station begins monitoring telemetry stream for `vehicle_mode == "RTL"`.
5. When `vehicle_mode == "RTL"` is observed, UI transitions to `CONFIRMED`.

## 5. Failure behavior

- If `vehicle_mode` does not transition to `RTL` within the confirmation window
  (default: configurable, recommended 5s), station raises a `CRITICAL` safety event
  and UI transitions to `FAILED`.
- `FAILED` state must be persistent and unmissable in UI.
- Re-triggering panic is allowed only by explicit operator action — no automatic background retry.

## 6. Audit requirements

Every panic trigger must produce an audit record including:

- operator session id (if available),
- timestamp,
- MAVLink command dispatched (`MAV_CMD_NAV_RETURN_TO_LAUNCH`),
- confirmation outcome (`CONFIRMED` or `FAILED`),
- time-to-confirmation or failure reason.

## 7. Verification checklist

- Panic control available across all primary UI states.
- Panic always dispatches `MAV_CMD_NAV_RETURN_TO_LAUNCH` to Pixhawk via MAVLink/UDP.
- Panic cannot be remapped by profile edit attempts.
- Confirmation is derived from telemetry `vehicle_mode` field only.
- Failure path produces `CRITICAL` visibility and audit log entry.
- Re-trigger requires explicit operator action.

## References

- `docs/adr/003-panic-button-system.md`
- `docs/design/PROTOCOL.md`
- `docs/design/Architecture.md`