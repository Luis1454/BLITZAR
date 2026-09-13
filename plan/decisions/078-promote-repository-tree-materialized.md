# Decision 078: Promote the repository tree to materialized

Status: accepted
Plan version: 1.0.62

## Context

Decision 076 frozen the target repository taxonomy in
`plan/repository_tree.json` while keeping `materialization: planned` so the
destination tree could be reached before the physical tree was promoted.
The migration moved the source side to the target responsibilities and
recorded the planned directory moves in the migration ledger. That left the
test side with eight directories that had no source mirror and therefore no
legal existence in the materialized policy:

- `tests/io/cli` and `tests/io/mpi` grouped CLI boundary tests without a
  mirror under `src/io`;
- `tests/mpi/fixture`, `tests/mpi/overlap`, `tests/mpi/rollback`, and
  `tests/mpi/wire` grouped MPI test responsibilities that were not declared
  under `src/mpi`;
- `tests/simulation/alloc` and `tests/simulation/lifecycle` grouped
  simulation-level tests without a `src/simulation` mirror.

## Decision

Promote the repository tree to `materialized` by reconciling the physical
test tree with the frozen taxonomy:

- The seven CLI boundary tests move to `tests/apps/blitzar`, the mirror of
  their `apps/blitzar` production responsibility. The two CLI-under-MPI output
  tests join them.
- `tests/mpi/fixture` flattens into `tests/mpi`: `MpiCases.hpp`,
  `MpiFixtureTest.cpp`, and `MpiAllocationTest.cpp` live at the pair root, and
  every test updates its fixture include from `mpi/fixture/MpiCases.hpp` to
  `mpi/MpiCases.hpp`.
- `tests/mpi/overlap/MpiOverlapTest.cpp` moves to `tests/mpi/exchange`, the
  mirror of its `src/mpi/exchange` responsibility. `tests/mpi/rollback` folds
  `MpiRollbackTest.cpp` into the `tests/mpi` root. `tests/mpi/wire`
  `MpiWireTest.cpp` moves to `tests/mpi/packets`, filling the mirror that
  `src/mpi/packets` had under `allowed_missing`.
- `tests/simulation/alloc/SimAllocationsTest.cpp` moves to the
  `tests/simulation` root. The empty `tests/simulation/lifecycle` directory is
  removed.
- Every remaining source responsibility directory without a test mirror is
  recorded explicitly in `allowed_missing` with a non-empty reason. The
  `src/mpi/packets` entry is dropped because its mirror is now materialized.

`materialization`, `migration.state`, and the accelerated
`target_test_entrypoints` reflect the physical tree, and the frozen plan is
bumped to 1.0.62.

## Acceptance

- `python -m tools.gates.repository_tree_gate --root . --plan`
- `python -m tools.gates.repository_tree_gate --root . --check`
- the complete static group through `tools.gates.quality_gate`;
- `python -m tools.format.format_clang_gate --root . --check`;
- the full CMake build and every registered CTest after the directory moves.