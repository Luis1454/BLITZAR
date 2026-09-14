# Decision 083: Hydrodynamics scope (P8-DEC-001)

Status: accepted
Plan version: 1.0.67

## Context

Issue 721 (`P8-DEC-001`) decides whether SPH, ISPH, YSPH, Eulerian
hydrodynamics, or a subset is part of the product, and ties every retained
model to equations, state ownership, conservation targets, and explicit
rejections so no ambiguous acronym or imported implementation survives in the
plan. The integrator scope (`INT-001`) and the comoving integrator
(`INT-005`) already closed the frame in which any hydro model must run.

## Decision

Retain five hydrodynamics families, all `retained-contracted` (deferred into
their implementation issues) except YSPH which stays `retained-confirmation-pending`:

- `wcsp-v1`: weakly-compressible SPH with a cubic B-spline kernel, Tait
  equation of state, linear-plus-quadratic artificial viscosity, and internal
  energy evolution; implemented through the existing `SolverForceEvaluation`
  boundary so the fixed KDK and comoving (INT-005) splits are unchanged.
- `isph-v1`: incompressible SPH projection (pressure Poisson on the
  deterministic neighbor graph, divergence-free correction).
- `ysph-v1`: entropy-formulation variant `P = A rho^gamma`; requires an
  unambiguous clean-room definition from issue `P8-SPH-007` before
  implementation.
- `eulerian-godunov-v1`: finite-volume Godunov with conservative cell state
  `Q = (rho, rho v, E)` on the P5 grid contract, selected Riemann solver, and
  deterministic reconstruction with limiters; CFL constrains the fixed step.
- `hybrid-sph-grid-v1`: particle-cell coupling with explicit double-counting
  prevention.

Recorded rejections: imported hydrodynamics implementations (clean-room),
zero-distance softening as a collision model (PHYS-001 non-goal), variable
timesteps (consistent with `plan/integration.json`), and SPH densities as a
gravity solver.

## Phase remapping

The frozen phase `P8` is `mpi-kdk-overlap-and-migration`. The issue series
`P8-SPH-*`, `P8-EULER-*`, `P8-HYBRID-*`, and `P8-QUAL-*` reuses the `P8` token
for hydrodynamics; to keep phase ids contiguous and unambiguous, the plan adds
phase `P10` (`hydrodynamics-and-hybrid`, depends on P5, P8, P9) and reserves
`TST-P10-*` for all hydrodynamics qualification.

## Constraints

- The gravity-only path is unchanged: fixed KDK and comoving trajectories, all
  solver families, PBC, MPI, restart, and reduction diagnostics keep their
  existing acceptance.
- This decision introduces no production code and no test id; hydrodynamics
  tests are registered by their implementation issues under `TST-P10-*`.
- The decision contract is frozen in `plan/hydrodynamics.json`.
