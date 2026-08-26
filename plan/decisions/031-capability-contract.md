# Decision 031: Capability and Unsupported-Feature Contract

Status: accepted
Issue: #611
Plan version: 1.0.14

## Context

The public ABI retains solver and backend hooks for forward compatibility, but
an enum or a compiled source file is not runtime evidence. PM, TreePM,
persistence, GPU FMM, and real multi-node execution are not implemented in the
current clean-room product.

## Decision

The repository keeps one machine-readable capability matrix in
`plan/capabilities.json`. It distinguishes implemented, locally qualified,
capability-gated, unsupported, and deferred behavior. The matrix records
compile requirements separately from runtime requirements and names the test or
CI evidence required for each claim.

The C ABI exposes `blitzar_get_capabilities_v2` as a versioned compile-contract
report. Its masks identify implemented solvers, explicitly unsupported solver
hooks, deferred features, and optional backend code compiled into the library.
The report is not a GPU or MPI runtime probe. Runtime backend selection remains
observable through `blitzar_simulation_backend`; hardware qualification remains
owned by the capability-gated CI and CTest paths.

`Simulation::SetSolver` must build a candidate before committing any solver
kind or variant state. PM, TreePM, and unknown values therefore return their
documented status without changing the previously valid solver.

`SnapshotHeader` remains a versioned state contract. No persistence claim may
be made until `src/io` and its corrupt/truncated/incompatible input tests are
materialized.

## Consequences

- Public consumers can inspect compile support without mistaking it for device
  execution.
- Unsupported solver requests are deterministic and transactional.
- Deferred roots remain absent and are checked against the matrix.
- CI summaries must distinguish compile-only, skipped-device, and executed-device
  outcomes.
