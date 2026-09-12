# Decision 076: Frozen Repository Tree Contract

Status: active  
Plan version: 1.0.60  
Scope: repository taxonomy, source/test symmetry, file naming, and Python layout

## Decision

The exact target repository taxonomy is frozen in
`plan/repository_tree.json`. That file is a source-of-truth plan artifact and
is validated by `CHK-P0-052` and `CHK-P0-053`.

The contract fixes the following boundaries:

- Responsibility aliases are stable and unique. `bh`, `cfg`, `diag`, `init`,
  `md`, `post`, `snap`, `stage`, and `tx` are the approved compact spellings.
- Solver variants live below one solver family root. FMM variants therefore
  use `src/solvers/fmm/{kifmm,...}` and the matching
  `tests/solvers/fmm/{kifmm,...}` hierarchy.
- Source and test responsibility directories are symmetric. A missing test
  directory is legal only through an exact `allowed_missing` entry with a
  non-empty reason. The entry becomes stale and fails as soon as both sides
  exist. Placeholder files such as `.keep` and `.gitkeep` are forbidden.
- C and C++ files are separated below `include/blitzar`, `tests/contracts`,
  and `examples`. They must not share a language-neutral directory.
- C, C++, and CMake test entrypoints end in `Test` before their extension.
  Python tests use the established `_test.py` suffix.
- Python production modules are grouped below the declared responsibility
  domains. Flat cross-domain modules are forbidden by the promoted tree gate.
- Directory and filename changes preserve responsibility markers while
  removing redundant path words. The target moves and the explicit planned
  exceptions are recorded in the machine-readable policy.

The policy is currently marked `materialization: planned`. This deliberately
freezes the destination tree before a migration changes include paths, CMake
registrations, test mappings, and Python imports. The existing physical tree
continues to be checked by the current directory-symmetry gate until the
migration is implemented. A future promotion to `materialized` must pass the
same policy in `--check` mode, including physical symmetry, target entrypoint
presence, language separation, forbidden-path rejection, and Python placement.

## Rationale

The previous layout mixed family names, responsibility names, and test roles.
That made equivalent source and test responsibilities drift independently and
made short names appear arbitrary. The frozen policy makes the directory carry
the broad responsibility and the filename carry only the local responsibility.
It also keeps the numerical and build work independent from a cosmetic tree
migration: no implementation claim is promoted merely because a destination
directory has been declared.

## Migration impact

The migration is one coherent repository-structure feature. It must update:

1. source and test paths in CMake and `plan/test_map.json`;
2. include paths, internal imports, and package/install paths;
3. architecture-review paths and filename exceptions;
4. Python module imports and quality-check commands;
5. the directory-symmetry policy, replacing the temporary current-layout pair
   with the promoted target pairs;
6. `plan/repository_tree.json`, changing both `materialization` and
   `migration.state` to `materialized` only after all target paths exist.

The migration must not introduce placeholder files, duplicate filename stems,
or a second source of truth for the destination tree. It is not complete while
the target gate is only passing in `--plan` mode.

## Acceptance tests

- `python -m tools.gates.repository_tree_gate --root . --plan`
- `python -m tools.gates.repository_tree_gate_test`
- `python -m tools.gates.directory_symmetry_gate --root . --check`
- `python -m tools.gates.directory_symmetry_gate_test`
- `python -m tools.gates.plan_check --root .`
- the complete static group through `tools.gates.quality_gate`;
- after promotion, `python -m tools.gates.repository_tree_gate --root . --check`.

