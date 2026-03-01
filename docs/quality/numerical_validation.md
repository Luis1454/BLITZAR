# Numerical Validation Baseline

This document defines acceptance-oriented numerical checks for astrophysics simulation outputs.

## Core Invariants

- Total energy drift bounded in reference scenarios.
- Center-of-mass drift bounded over long runs.
- Time-step convergence trends consistent with integrator expectations.
- Radiation exchange remains finite and physically coherent.

## Reference Tests

- `PhysicsTest.TST_UNT_PHYS_002_EnergyConservation`
- `PhysicsTest.TST_UNT_PHYS_006_EnergyConservationHighMassNoSph`
- `PhysicsTest.TST_UNT_PHYS_003_CenterOfMassDrift`
- `PhysicsTest.TST_UNT_PHYS_004_TimeStepConvergence`
- `PhysicsTest.TST_UNT_PHYS_008_RadiationExchangeConservation`

## Acceptance Policy

- Threshold changes require explicit rationale and reviewer approval.
- Validation updates must include traceability entries for impacted requirements.
- Non-deterministic failures block promotion to release lanes until root cause is identified.
- Qualification evidence must be produced under `prod` profile constraints.

## Cross-Backend Rule

- CPU/CUDA/OpenCL parity targets must be defined before enabling a backend in release lanes.
- New backend enablement requires:
  - tolerance envelope definition,
  - benchmark dataset reference,
  - nightly comparison evidence against existing qualified backend.

## Reproducibility Rule

- Reference scenarios must support repeated runs with stable outcomes under pinned toolchain and build flags.
- Any backend-specific deviation requires documented tolerance and explicit approval in quality artifacts.
