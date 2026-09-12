# Decision 077: P9 portable restart implementation and resolved runtime identity

Status: accepted
Issue: OPS-002
Plan version: 1.0.61

## Context

Decision 073 froze P9/OPS-002 as the planned successor to the V1 particle-only
restart contract. The V1 payload cannot carry integrator, RNG, units, execution
math, backend identity, global domain, or source ownership, so a rank-count
changing restart and a resolved fast-backend identity were unowned gaps. This
decision begins implementation on the CPU strict lane as planned; MPI
rank-change and HIP device identity remain capability-gated evidence lanes.

## Decision

Add the versioned V2 backend-neutral snapshot codec under
`src/io/snap/codec`:

- `SnapshotV2State` defines the tagged section schema: particle state,
  integrator state, RNG state, unit system, execution math state, resolved
  backend identity, global domain, and source ownership. Derived state is
  never serialized.
- `SnapshotV2Writer` and `SnapshotV2Reader` implement the tagged wire format
  `magic|version|section_count` followed by tagged sections and a trailing
  checksum. The reader validates the full frame and checksum before decoding,
  rejects unknown sections and incompatible versions explicitly, and leaves
  the destination unchanged on any rejection.
- `SnapshotRepartition` assigns every stable particle ID deterministically to
  exactly one destination rank for a changed rank count.
- `MetadataExecution` gains an explicit resolved-identity contract: fast
  manifests may begin `runtime-selected`, but a completed persistent manifest
  must record `cpu` or `hip` with its device boundary.

The five OPS-002 acceptance cases are registered as `TST-P9-001` through
`TST-P9-005` against the `blitzar_snapshot_v2_test` entrypoint using the
`roundtrip`, `parity`, `repartition`, `identity`, and `reject` selectors.

## Acceptance

- `TST-P9-001` complete-state round-trip preserves all eight V2 state areas.
- `TST-P9-002` same-rank strict CPU save/reload/resume is byte-identical.
- `TST-P9-003` deterministic repartition covers every stable ID exactly once.
- `TST-P9-004` completed fast manifests reject `runtime-selected` identity.
- `TST-P9-005` checksum, truncation, unknown-section, and version errors leave
  the destination state unchanged.
- The static gate group, the full build, and the registered CTest suite remain
  green; MPI rank-change and HIP device identity stay capability-gated.
