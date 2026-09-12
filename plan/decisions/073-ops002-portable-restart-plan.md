# Decision 073: OPS-002 portable restart and runtime identity plan

Status: accepted
Issue: OPS-002
Plan version: 1.0.57

## Context

The P6 reproducibility contract now makes the current boundary honest: V1
snapshots contain particle state, distributed restart requires the same rank
count, and a fast manifest may use `runtime-selected` before the execution
backend is known. Those boundaries were documented, but the follow-on work was
only named in prose and had no phase, owner, contract, or acceptance cases.

## Decision

Add P9 as a planned phase after the P4, P6, P7, and P8 contracts. P9 is owned
jointly by the persistence and parallel-maintainer boundaries and is specified
by `plan/ops002.json`.

P9 defines a versioned V2 backend-neutral restart state. Its conceptual state
includes particle state, integrator state, RNG state, units, execution math
state, resolved backend identity, global domain bounds, and source ownership.
Stable particle IDs remain authoritative. Derived trees, ghost state, and other
execution caches are rebuilt after decode and redistribution.

P9 permits a destination rank count different from the source rank count. The
source rank count, global bounds, and source domains are serialized and
validated before deterministic repartition. Decode, repartition, and commit are
transactional. V1 remains readable only under its existing same-rank,
particle-state contract and must reject a portable rank-count-changing restart.

Fast output may publish provisional `runtime-selected` metadata before backend
selection, but a completed persistent manifest must record the resolved `cpu` or
`hip` backend and its device boundary. Strict output remains `cpu`/`host-cpu`.
Fast output remains non-bitwise regardless of the resolved backend.

This is a planning change only. P9 acceptance cases are registered as CTest
tests when implementation starts; no current test or capability is promoted by
this decision.

## Acceptance

- `plan/ops002.json` defines the owner, oracle, state fields, migration rules,
  rank-count change contract, backend identity contract, and five acceptance
  cases.
- The manifest exposes P9 as `planned` and preserves P0-P8 implementation
  statuses.
- The V1 P6 contract explicitly links its particle-only and same-rank limits
  to P9 rather than treating them as unowned gaps.
- The final plan states that physical GPU, MPI, sanitizer, and multi-node
  qualification remain capability-gated or non-claims according to their
  existing lanes.
