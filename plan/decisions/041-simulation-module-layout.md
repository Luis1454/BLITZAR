# Decision 041: Simulation Module Layout

Status: accepted
Issue: #653
Plan version: 1.0.25

## Context

The simulation implementation had one input directory containing unrelated
responsibilities: directive parsing and semantic configuration, deterministic
initial-state generation, transient particle staging, and Sim state transfer.
That name described a call direction rather than an ownership boundary and
made new configuration and persistence work likely to accumulate in one
directory.

## Decision

Remove src/simulation/input and use three responsibility boundaries:

- src/simulation/config owns SimConfigFile, SimConfigValue,
  SimConfigDirective, SimConfigSimulation, SimConfigPhysics, SimConfigOutput,
  SimConfigDiagnostics, and SimConfigRun.
- src/simulation/initialization owns SimConfigState and SimInputStage. These
  components turn validated configuration or caller-provided particle views
  into bounded initial state ready for Sim.
- src/simulation/state owns SimParticleState, SimParticleSet, and
  SimParticleGet. This is the state ownership and state-transfer boundary of
  Sim.

The move is structural only. It does not add compatibility include shims,
duplicate implementations, change symbols, change the public SDK, or change
runtime behavior. CMake, tests, plan roots, quality rules, and architecture
reviews must point to the owning directories.

## Consequences

Configuration policies can evolve without being mixed with particle
initialization or state transfer. Initialization remains explicit and
bounded, while the existing Sim state contract remains in one domain.
The old input path is forbidden so stale includes fail immediately instead of
silently preserving the previous layout.
