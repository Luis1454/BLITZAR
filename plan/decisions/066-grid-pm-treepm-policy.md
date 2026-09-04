# Decision 066: CPU Grid, PM, and TreePM Promotion

Status: **Active**
Plan version: **1.0.50**

## Context

The resource contract already reserves a Grid resource for PM and an Octree
plus Grid resource pair for TreePM, but those solver families have remained
explicitly unsupported. The next implementation must preserve the Direct CPU
path as the numerical reference, keep grid ownership separate from solver
execution, and avoid introducing a public settings ABI.

## Decision

Materialize `src/grid`, `src/solvers/pm`, and `src/solvers/treepm` as CPU
production roots. The first implementation uses a bounded 8 by 8 by 8 finite
non-periodic mesh. Its domain is the source axis-aligned bounding box expanded
by one length unit; degenerate axes use a two-unit interval centered on the
source coordinate. Source
mass is deposited with cloud-in-cell weights, the softened Newton field is
computed by a deterministic discrete Green convolution, and the field is
interpolated with the same clamped trilinear policy. Grid storage is allocated
by the solver constructor and steady-state evaluation is allocation-free after
preparation.

TreePM composes the existing Barnes-Hut solver and the PM solver. Its frozen
baseline combines their staged fields with weights `0.5` and `0.5`; neither
algorithm is copied into the other root. Any physical split-function or GPU
dispatch is a later plan change.

The CPU capability is promoted only for single-rank execution. Distributed PM
and TreePM are rejected during `Sim::Step` preflight, before transaction,
migration, or state mutation, because P5 does not define global mesh ownership
or a mesh halo contract. The C ABI remains unchanged; the existing solver enum
values become supported CPU selections and no new public settings are exposed.

## Acceptance Evidence

- `TST-P5-001` validates layout, clamped boundaries, CIC mass conservation,
  generation invalidation, and bounded resource capacity.
- `TST-P5-002` validates PM CPU force repeatability, finite output, allocation-
  free steady state, and comparison with the Direct CPU reference.
- `TST-P5-003` validates TreePM composition, tree/grid resource ownership,
  repeatability, and Direct CPU comparison.
- `CHK-P0-046` validates this frozen P5 contract and `CHK-P0-047` validates its
  parser fixtures.

The qualification does not claim GPU speedup, distributed parity, or a
production FFT implementation.

## Migration Impact

Existing `BLITZAR_SOLVER_PM` and `BLITZAR_SOLVER_TREEPM` values, C ABI layout,
and C++ wrapper spellings remain unchanged. Capability reporting changes from
unsupported to implemented CPU-qualified after the three acceptance tests
pass. The prior unsupported-selection assertions are replaced with successful
single-rank selections; distributed rejection remains an internal execution
boundary.
