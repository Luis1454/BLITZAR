# Decision 085: SPH particle and neighbor state contract (P8-SPH-001)

Status: accepted
Plan version: 1.0.69

## Context

Issue 719 (`P8-SPH-001`) fixes the SPH particle-state architecture before any
hydrodynamics implementation: fields, smoothing length, support radius,
neighbor views, and ownership rules, with units, lifetime, and initialization
for every field, bounded non-allocating deterministic neighbor iteration, and
defined empty, boundary, and duplicate-neighbor cases. It must work for local
and MPI-owned particles and stay separate from gravity state while sharing
particle identity. No equation of state is selected by this contract.

## Decision

Freeze the contract in `plan/sph_particles.json`. Per-particle density,
pressure, specific internal energy, and smoothing length are SoA hydro
execution state sharing the existing particle identity and active mask; mass,
position, and canonical velocity remain untouched. Density is the deterministic
neighbor summation, smoothing length and density obey a recorded
fixed-point consistency `h_i = eta (m_i / rho_i)^(1/3)` with declared
iterations, and support radius is exactly `2 h_i`. Neighbor views reuse the
deterministic local-neighbor index contract (cell-linked candidate) with the
skin; they never allocate and never build an index in the hydro path. Empty
support rejects and rolls back on zero/non-finite density; periodic and finite
boxes follow the existing boundary contracts; duplicate directed pairs are
index quality errors. MPI-owned particles carry hydro state through the same
index; nothing is ghosted. The ysph-v1 entropy slot is reserved with no
lifetime until P8-SPH-007 freezes its definition.

## Constraints

- Hydro evaluation stays on the existing SolverForceEvaluation boundary; no
  new solver entry point and no gravity-path change.
- Gravity-only trajectories and all pre-existing test ids are unchanged.
- New hydro tests use the TST-P10-* namespace.
- The concrete EOS remains open and is selected by P8-SPH-004.
- This contract introduces no production code.
