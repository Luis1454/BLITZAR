# Decision 087: Production SPH Density Summation (P8-SPH-003)

Status: active. Plan version: 1.0.71. Issue: 723.

## Decided

`src/physics/sph/SphDensity.{hpp,cpp}` computes the deterministic SPH density
summation from the frozen dimensional contract (`sph_particles.json`,
`hydrodynamics.json`, and the `neighborhood.json` neighbor geometry).

- Kernel: the normalized cubic B-spline (Monaghan) weighting
  `W(r,h) = (1/(pi h^3))[1 - 3/2 q^2 + 3/4 q^3]` for `0 <= q < 1`,
  `(1/(4 pi h^3))(2 - q)^3` for `1 <= q <= 2`, `0` beyond, with `q = r/h`.
  The support is the compact `2 h` shell and the integral over space is one,
  so masses and densities keep physical mass-over-volume units.
- Summation: `rho_i = sum_j m_j W(|r_ij|, h_i)` runs over the strictly
  ascending neighbor lists from `NeighborIndex` (built with radius `2 h`),
  giving a bit-reproducible accumulation order. The index excludes the self
  pair, so the self term `m_i W(0, h_i)` is added explicitly and is part of
  the neighborhood. Per-particle, variable smoothing length `h_i` is an input;
  the `h(rho)` fixed-point remains an execution-level policy and is exercised
  by later chantiers, not re-derived here.
- Normalization: `normalize_boundary=false` keeps the physical summation
  (truncated at finite-box boundaries, deterministic). `normalize_boundary=true`
  applies the Shepard correction `rho_i = (sum m_j W_ij)/(sum W_ij)`, which
  restores exact uniform-field values in boundary-truncated neighborhoods; the
  corrected output carries particle-mass units and is qualified against the
  two-body and boundary oracles.
- Fail-safes precede any output write: span mismatch and non-finite, negative,
  or non-positive masses and smoothing lengths are `INVALID_ARGUMENT`; empty
  indexed support, non-positive neighborhood mass, and non-finite or
  non-positive results are `SINGULARITY`. An unbuilt index is rejected.

## Qualified

- `TST-P10-002` (`SphDensityTest`): kernel spot values and support; exact
  two-body summation and Shepard equivalence to the particle mass; uniform
  lattice interior within 1e-2 of the analytic value; Shepard boundary across
  the whole lattice within 1e-10 relative; a gentle analytic sinusoid within
  1e-2 in the interior; empty, zero-mass, negative-mass, degenerate smoothing
  length, span-mismatch, and unbuilt-index failure without publishing any
  output value.
- Density is derived state (restart-transparent): only positions, masses, and
  smoothing lengths are canonical; snapshots do not store density.

## Corrected

- The P8-SPH-002 reference for equation-of-state selection in
  `sph_particles.json`, `plan/decisions/085-...`, and `PLAN.md` was wrong: the
  EOS chantier is `P8-SPH-004` (issue 722). The three records now name
  `P8-SPH-004`.
