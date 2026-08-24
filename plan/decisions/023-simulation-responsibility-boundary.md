# Decision 023: Simulation Responsibility Boundary

Status: accepted
Issue: #577
Plan version: 1.0.10

## Context

`src/sdk/Simulation.cpp` mixed construction, solver configuration, particle state
transfer, and local/distributed KDK execution. The previous extraction of
The first internal extraction used redundant server-oriented prefixes for SDK
implementation files and helper types. Those names leaked an obsolete server
boundary into the clean-room SDK and obscured the responsibility owned by the
`src/sdk` module.

## Decision

Keep `Simulation` as the composition root and distribute its implementation by
behavior:

- `Config.cpp` owns solver and runtime configuration mutators.
- `Particles.cpp` owns particle input commit and state output gathering.
- `Step.cpp` owns KDK dispatch, distributed transaction, and migration.
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
