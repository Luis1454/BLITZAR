# Decision 048: Conservation Metrics Contract

Status: accepted
Issue: #644
Plan version: 1.0.32

## Context

The KDK numerical test contained the only implementation of momentum and energy
measurement. That code was tied to one two-particle fixture, did not provide a
reusable failure contract, and could not be qualified independently from the
integrator. The existing `ParticleStateView` also supports a target count and an
optional larger source view for force computation, which must not be silently
interpreted as a complete conservation state.

## Decision

- `src/physics/conservation/ConservationMetrics.hpp` and
  `ConservationMetrics.cpp` own the reusable internal physics boundary. The
  output is a value type containing kinetic, potential, and total energy plus
  the three momentum components. The computation has three arguments and does
  not widen the public SDK ABI.
- Metrics require a complete state: `SourceCount()` must equal `count`, all
  state values must be finite, and masses must be non-negative. Invalid input
  returns `BLITZAR_STATUS_INVALID_ARGUMENT` without modifying the output.
- Kinetic energy and momentum accumulate over `state.count`. Potential energy
  uses the deterministic `i < j` pair order, `GravityParameters` effective
  units, and `GravityLaw` softening validation. A pair containing a zero mass
  contributes zero before distance or singularity evaluation. Two positive
  masses at the same position with zero softening return
  `BLITZAR_STATUS_SINGULARITY`.
- The implementation uses a local candidate result and commits it only after
  every intermediate value is finite. It performs no dynamic allocation and no
  unordered or parallel reduction.
- CLI diagnostics, CSV output, filesystem persistence, MPI aggregation, and
  HIP execution remain outside this issue.

## Consequences

The direct CPU reference, Barnes-Hut, and FMM paths can be qualified against
one production metrics contract without duplicating formulas in tests. The
complete-state restriction prevents remote source buffers from being silently
omitted; a future local or distributed diagnostic must use a distinct contract.
`TST-P6-005` covers value calculation, empty input, singularity, zero-mass
handling, invalid boundaries, and solver integration. `TST-P1-002` now uses the
same production calculation for long-run KDK conservation.
