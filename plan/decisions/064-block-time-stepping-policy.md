# Decision 064: Block Time-Stepping Qualification Policy

- Status: active
- Scope: clean-room issue #676
- Plan version: 1.0.48

## Context

Block time stepping can reduce the number of scheduled force evaluations for
particles with different time scales, but it also changes synchronization,
ownership migration, rollback, restart, and interaction-list semantics. The
current production integrator is fixed-step Leapfrog KDK. Introducing partial
asynchronous updates before those contracts are proven would make numerical
and distributed failures difficult to recover.

## Decision

Issue #676 is a qualification boundary, not a production integrator change.
The test-only `block-kdk-schedule-v1` model uses immutable power-of-two time
bins from zero through three. Active particles are emitted in stable particle
index order. Ownership changes are applied only at the versioned
synchronization boundary, and a captured schedule state contains the tick,
ownership, counters, and event hash needed for exact restart and rollback.

The existing fixed-step `fixed-kdk-v1` path remains the numerical and
production reference. The qualification reports a modeled work reduction and
the scheduling-loop timing, but neither is presented as an end-to-end force
speedup. Physical energy and momentum qualification remains owned by the
existing fixed KDK tests. The candidate is rejected for promotion because no
production block-force path or complete end-to-end gain has been demonstrated.

No partial asynchronous update is added to `KdkLeapfrog`, `Sim`, the MPI
transport, the snapshot ABI, or the public SDK.

## Invariants

- Every candidate bin is a bounded power-of-two interval.
- Active work lists are deterministic and ordered by stable particle index.
- Ownership changes occur only on a synchronization tick.
- A failed block attempt restores all schedule state before retry.
- Restart from a captured state equals uninterrupted schedule execution.
- The qualification does not mutate the workload or production particle state.
- The unmodified fixed-step KDK remains the production integrator.

## Evidence

`TST-P1-008` runs heterogeneous, clustered, and migration workloads through
the bounded schedule model. It checks modeled work reduction, exact hashes,
ledger conservation, migration boundaries, restart, rollback, and input
non-mutation. `CHK-P0-042` validates this contract and `CHK-P0-043` validates
the external evidence parser and fixtures. `TST-P1-002`, `TST-P8-001`, and
`TST-P8-002` remain the physical KDK and distributed rollback references.
