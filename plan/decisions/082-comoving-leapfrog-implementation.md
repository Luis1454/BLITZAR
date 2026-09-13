# Decision 082: Comoving Leapfrog implementation (INT-005)

Status: accepted
Plan version: 1.0.66

## Context

Issue 739 (`INT-005`) implements the retained `comoving-kdk-v2` integrator
frozen by decision 081. The integrator must add expanding-universe support
without disturbing the sole production integrator `fixed-kdk-v1`, whose
trajectory remains the numerical reference.

## Decision

Implement the cosmological Leapfrog KDK on co-moving coordinates in
`src/integration/comoving` behind the existing `SolverForceEvaluation`
boundary. The SoA velocity field stores the canonical comoving momentum
`w = a * v`; the Hubble drag is removed from the kick by construction, so the
integrated equation is `dw/dt = -g` and the drift is `x += dtau * w` with
`dtau = integral_of(dt / a)`. With a `Static` background (`a == 1`), `dtau == dt`
exactly and the comoving integrator reproduces fixed KDK bit-exactly, which is
enforced by a strict parity oracle test.

The Einstein-DeSitter background `a(t) = (t / t0)^(2/3)` is implemented in
`ComovingBackground.cpp` with restart transparency: the expanding frame is
fully described by the single scalar cosmic time `t`, so scale factor and
redshift recover deterministically. Validation rejects non-finite states,
invalid backgrounds, and non-positive timesteps or cosmic times; force-failure
rollback restores state and cosmic time exactly.

## Acceptance evidence

- `TST-P1-010` (`blitzar_comoving_background_test`) checks static and EdS
  background semantics against analytic values.
- `TST-P1-011` (`blitzar_comoving_leapfrog_test`) checks: static bit-parity
  with fixed KDK over 4096 steps; empty-universe canonical-momentum invariance
  and analytic comoving drift; second-order convergence of an expanding
  two-body orbit against fine-step references; comoving momentum conservation;
  validation rejection of invalid inputs; and exact rollback on a failing force
  phase.
- Fixed KDK trajectories and all pre-existing test ids are unchanged.

## Constraints

- No new solver entry point, buffer field, or public API surface; the public
  boundary is untouched by this decision.
- `block-kdk-schedule-v1`, variable-step timesteps, higher-order symplectic,
  explicit Runge-Kutta, implicit integrators, and imported schemes remain
  rejected as recorded in `plan/integration.json`.
