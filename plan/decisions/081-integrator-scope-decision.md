# Decision 081: Integrator scope decision (INT-001)

Status: accepted
Plan version: 1.0.65

## Context

Issue 713 (`INT-001`) freezes which integrators beyond the fixed-step Leapfrog
KDK are product requirements and records every rejected candidate. The
decision also gates the later hydrodynamics and physics chantiers
(`P8-DEC-001`, `PHYS-001`), which depend on a closed integrator contract.

## Decision

Keep `fixed-kdk-v1` (physical coordinates, Plummer softening,
`src/integration/kdk`) as the sole production integrator. Retain one additional
integrator: `comoving-kdk-v2`, a cosmological Leapfrog KDK on co-moving
coordinates with peculiar velocities, defined independently by
`plan/integration.json` and decomposed into implementation issue `INT-005`
(#739).

Record the rejected candidates explicitly and without ambiguity: block-time
scheduling is an evidence-only P1 proxy that is not promoted; variable-step
individual timesteps, fixed-step higher-order symplectic methods, explicit
Runge-Kutta, implicit integrators, and any imported Gadget-style scheme are
rejected for production. The co-moving scheme is defined by this contract only,
preserving the clean-room rule.

The contract reuses the existing `SolverForceEvaluation` boundary, keeps the
fixed-KDK checkpoint, rollback, finite-state, no-hot-path-allocation, and
argument-count rules, and stays domain-agnostic so Direct, Barnes-Hut, FMM,
KIFMM, PM, and TreePM remain usable through the same force path.

## Constraints

- No production code or test id is introduced by this decision; the co-moving
  oracle tests are registered by `INT-005`.
- Fixed KDK remains the numerical reference; no new integrator changes the
  existing gravity-only trajectory.
