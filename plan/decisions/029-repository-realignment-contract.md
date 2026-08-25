# Decision 029: Repository Realignment Contract

Status: accepted
Issue: #598
Plan version: 1.0.12

## Context

The clean-room tree has completed P0 through P4 and the local P7/P8 work, while
P5 and P6 remain deferred. The previous manifest made P7 depend on deferred P6
and the decision records did not distinguish accepted rationale from active,
superseded, or historical lifecycle state.

## Decision

The repository uses four planning sources of truth:

- `PLAN.md` defines product boundaries and human-readable contracts.
- `plan/manifest.json` defines roots, phase dependencies, and phase status.
- `plan/quality.json` defines tests, checks, and architecture thresholds.
- `plan/decision-index.json` defines decision lifecycle state.

P7 and P8 depend on the qualified P3/P4 solver and runtime contracts. They do
not depend on the deferred P5/P6 roots. P5 and P6 remain deferred until their
production roots, owners, and acceptance tests are materialized.

Complete filenames, including extensions, must be unique. Matching `.cpp` and
`.hpp` stems are allowed only for the same primary type or an explicitly
documented public/configuration pair. Internal callables remain bounded to
four parameters; frozen C ABI V1 functions are tracked exceptions.

## Consequences

- Capability reports can distinguish implemented, capability-gated, deferred,
  and unsupported behavior.
- Decision history can be retired without deleting its rationale.
- Future plan changes must update both the decision record and its lifecycle
  index entry.
- The next quality-gate issue will make these invariants executable.
