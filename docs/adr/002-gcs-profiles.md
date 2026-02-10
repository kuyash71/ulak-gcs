# ADR-002: GCS Profiles and Policy Mapping

- Status: Accepted
- Date: 2026-02-10
- Owners: ULAK GCS maintainers

## Context

The station must support both simulation and real vehicle operations without forking business logic. Teams need to tune safety behavior without recompiling the application. At the same time, core safety invariants (panic behavior, severity model) must remain stable.

## Decision

1. GCS behavior is policy-first and profile-driven.
- Runtime behavior is controlled by profile data loaded from `config/profiles/*.json`.
- Profile content maps `event_code -> severity -> action`.

2. Severity model is fixed in MVP.
- Allowed severity values are exactly `WARN`, `ERROR`, `CRITICAL`.
- Profiles can map events to one of these levels, but cannot add new severity levels.

3. Default profile is mandatory and protected in UI workflows.
- Default profile exists as a safe fallback.
- UI cannot delete the default profile.
- If an active profile is invalid or unavailable, system falls back to default with an audit event.

4. Panic behavior is outside profile control.
- Profiles MUST NOT override panic command semantics.
- Panic remains fixed to `PANIC_RTL` contract.

5. Policy contract aligns with protocol and architecture docs.
- Protocol categories remain aligned with `docs/design/PROTOCOL.md`.
- Core behavior boundaries remain aligned with `docs/design/Architecture.md`.

## Consequences

- Operators can switch behavior by profile without code changes.
- Safety-critical invariants stay deterministic across deployments.
- Validation and audit requirements increase (schema checks + profile provenance tracking).

## Implementation notes (MVP)

- Profiles should carry `schema_version`, `profile_id`, `display_name`, `mappings`, and `defaults`.
- Every profile change and fallback event must be audit-logged with timestamp and reason.
- Invalid profile data must never be silently accepted.

## References

- `docs/design/Architecture.md`
- `docs/design/PROTOCOL.md`
- `docs/spec/exception-handling.md`
- `docs/adr/003-panic-button-system.md`
