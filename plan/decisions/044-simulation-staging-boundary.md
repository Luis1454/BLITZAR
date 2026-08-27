# Decision 044: Simulation Staging Boundary

Status: accepted
Issue: #656
Plan version: 1.0.27

## Context

Decision 041 removed the misleading `src/simulation/input` directory, but it
left configuration-generated initial state and transient caller-particle
staging in the same `initialization` responsibility. These lifetimes and
failure paths differ: configuration state is a CLI construction result, while
particle staging is a temporary candidate used by `Sim::SetParticles` before
MPI distribution and state commit.

## Decision

Keep the old `src/simulation/input` path absent and use these boundaries:

- `src/simulation/config` owns directive parsing and semantic configuration.
- `src/simulation/initialization` owns `SimConfigState` and deterministic state
  generation from a validated `SimConfigRun`.
- `src/simulation/staging` owns `SimParticleStage` and `StageParticles`, which
  validate and copy a caller particle view into bounded temporary SoA storage.
- `src/simulation/state` owns the persistent simulation state and its transfer
  operations.

The change is structural and naming-only. It does not change the public SDK,
particle validation rules, MPI packet protocol, distribution order, rollback
behavior, or numerical algorithms. No compatibility include or duplicate
implementation is retained at the old path.

## Consequences

The initialization directory now has one purpose: constructing deterministic
state from configuration. The staging directory makes the temporary ownership
boundary explicit and leaves `SetParticles` responsible for committing only
validated data. Future input adapters can target the staging contract without
reintroducing a catch-all input directory.
