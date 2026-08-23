# Decision 023: Simulation Responsibility Boundary

Status: accepted
Issue: #577
Plan version: 1.0.7

## Context

`src/sdk/Simulation.cpp` mixed construction, solver configuration, particle state
transfer, and local/distributed KDK execution. The previous extraction of
`SrvState`, `SrvTransaction`, and `SrvDispatch` did not remove the remaining
responsibility overlap.

## Decision

Keep `Simulation` as the composition root and distribute its implementation by
behavior:

- `SrvConfig.cpp` owns solver and runtime configuration mutators.
- `SrvParticles.cpp` owns particle input commit and state output gathering.
- `SrvStep.cpp` owns KDK dispatch, distributed transaction, and migration.
- `Simulation.cpp` owns construction, narrow state accessors, and status storage.

The internal API accepts cohesive view/configuration records instead of
multi-array convenience overloads. No new callable exceeds four parameters.
Solver reconstruction is shared through `RebuildSolver` so configuration
mutators do not duplicate lifecycle logic.

## Consequences

The public C ABI and C++ RAII facade remain unchanged. Internal tests construct
`ParticleStateView` and `ParticleOutputView` explicitly. The NVIDIA path uses
the existing CUDA compatibility layer without linking an AMD HIP host target,
which prevents contradictory platform macros when compiling `.hip` sources with
`nvcc`.
