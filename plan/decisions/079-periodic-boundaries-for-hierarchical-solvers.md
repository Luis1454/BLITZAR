# Decision 079: Periodic boundaries for hierarchical solvers

Status: accepted
Plan version: 1.0.63

## Context

Issue 741 (`P3-PBC-001`) requires periodic-box interactions for the Direct,
Barnes-Hut, FMM, and KIFMM CPU solvers. The existing Octree hierarchy is built
over finite open-boundary coordinates; there is no periodic wrapping,
minimum-image displacement, or image-aware cell acceptance anywhere in the
solver path. The clean-room plan freeze covered the finite grid resource
(`plan/grid.json`), the local-neighbor window (`plan/neighborhood.json`), and
the multipole and BVH candidates, but no periodic domain.

A pure `1/r^2` gravitational lattice sum is conditionally convergent, so the
contract must bound the interaction window and defer the long-range lattice
regularization instead of silently claiming a physical periodic gravity model.

## Decision

Freeze `plan/periodic.json` and register a new physics responsibility:

- `src/physics/periodic/PeriodicDomain` is a bounded value type owning box
  origin, per-axis size, and an enabled flag. It exposes box wrap, per-axis
  minimum-image fold, a deterministic 27-copy image-region enumeration (the
  central copy plus the 26 surrounding regions), and periodic cell
  containment. It performs no allocation and reduces to the identity when
  boundaries are disabled.
- The interaction window is bounded by `Rc <= min(Lx, Ly, Lz) / 2` so each
  distinct source has at most one image inside `Rc`; the minimum-image
  displacement is that image. Evaluating against the central copy plus the 26
  surrounding regions equals the Direct periodic reference after window
  reduction.
- The solver path reuses the single Octree built over the wrapped box: leaf
  sources, multipole displacements, far-cell equivalent points, and cell
  containment are folded through the periodic domain. No per-image hierarchy
  is created. The non-periodic path is bit-identical to the existing CPU lane.
- `SolverForceEvaluation` and the typed `Direct` and `Tree` requests gain a
  trailing `const PeriodicDomain*` member (defaulted to null) so the provider
  traits forward the domain without breaking aggregate initialization.
- Long-range lattice regularization (Ewald, P3M mesh, TreePM long range) is
  explicitly not part of this contract; the grid mesh and the MPI halo,
  migration, restart, and rank-count lanes remain coordination points shared
  with issues 695 and the P7/P8 parallel lanes. The MPI lane is capability-
  gated on the single-rank CPU qualification path.

The plan is bumped to 1.0.63, the manifest registers
`plan/periodic.json` and the two mirrored directories, the naming registry
gains the `src/physics/periodic` and `tests/physics/periodic` prefix, and the
qualification is registered as `TST-P3-010` through `TST-P3-018` on the CPU
strict lane.

## Acceptance

- `python -m tools.gates.plan_check`
- `python -m tools.gates.repository_tree_gate --root . --plan`
- `python -m tools.gates.repository_tree_gate --root . --check`
- the complete static group through `tools.gates.quality_gate`;
- the full CMake build and `TST-P3-010` through `TST-P3-018` in CTest;
- the physical parity and no-double-count cases demonstrate `PBC-AC-001`
  through `PBC-AC-008`; `PBC-AC-009` registers as a capability-gated lane.
