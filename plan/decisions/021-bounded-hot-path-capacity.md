# Decision 021: Bounded Hot-Path Capacity

Status: accepted
Plan version: 1.0.6

## Decision

Execution capacities are established when `Simulation` is constructed. Packet
buffers expose `ResizeBounded`, and direct-solver staging is prepared before
the first step. MPI migration, gather, packet transport, ghost transport, and
distributed rollback reuse persistent buffers and reject requests beyond their
capacity before publishing state.

Standalone transport tests may prepare a smaller explicit capacity; they do
not receive an implicit allocation from an execution method.

## Consequences

- Steady-state CPU and distributed steps do not resize vectors or create local
  packet workspaces.
- Migration and ghost outputs are bounded by the simulation particle arena.
- A capacity failure is reported as `BLITZAR_STATUS_INVALID_ARGUMENT` before
  particle state is committed.
- Configuration paths such as `SetParticles` may still allocate their input
  staging because they are not part of the step hot path.
