# Clean-Room Contribution Rules

This repository is implemented from `PLAN.md` and `plan/manifest.json` only.
Do not inspect, copy, translate, or use the old BLITZAR repository as context.

`PLAN.md` and every `plan/*.{json,md}` file are the frozen source of truth. Read
the relevant `plan/*.json` contract before changing a subsystem; the JSON gates
encode more detail than the prose.

## Commands

Run from the repository root. Build out of tree; local builds use
`../.blitzar-build` (workspace policy in `plan/workspace.json`). `/build/` and
`/build-*/` are ignored and must never be committed (`build-qualification/` is
an ignored local qualification tree, not source).

```sh
cmake -S . -B ../.blitzar-build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DBLITZAR_BUILD_TESTS=ON -DBLITZAR_BUILD_CLI=ON -DBLITZAR_BUILD_EXAMPLES=ON
cmake --build ../.blitzar-build --parallel
ctest --test-dir ../.blitzar-build --output-on-failure
ctest --test-dir ../.blitzar-build -R '^TST-P1-002$' --output-on-failure   # single test id
```

- Static policy gates (no build required; the lint/typecheck equivalent):
  `python -m tools.gates.quality_gate --root . --group static`.
- Format check, pinned clang-format `22.1.8`, never rewrites:
  `python -m tools.format.format_clang_gate --root . --check`.
- Statement-grouping checker / writer:
  `python -m tools.format.format_blocks --check` or `--write [paths...]`.
- Architecture report: `python -m tools.architecture.architecture_report --root . --all --check --quiet`.

## Build Flags And Quirks

- `BLITZAR_HIP_MODE`, `BLITZAR_MPI_MODE`, `BLITZAR_HDF5_MODE` are
  `AUTO|ON|OFF`; defaults are `AUTO` and silently fall back when the dependency
  is absent. `BLITZAR_HIP_MODE=ON` requires `HIP_PLATFORM=amd|nvidia` and
  `hipcc`/`nvcc`.
- `BLITZAR_BUILD_SHARED=ON` is incompatible with tests and with the CLI
  (internal composition and internal symbols); CMake fails fast.
- The C ABI test target builds `.c` sources but links as C++ (`LINKER_LANGUAGE CXX`).

## Tests

Test ids (`TST-P*`) are duplicated across four places and must stay in sync:
`plan/quality.json` (id + command), `cmake/test_registration.cmake`
(`add_test`/`ctest -R` names), `cmake/test_targets.cmake` (executable), and
`plan/test_map.json` (entrypoint/responsibility file). C/C++ test entrypoints
end in `Test`; Python tests end in `_test.py`. Evidence contracts live in
`tools/evidence/*_contract.py`; `plan/quality.json` uses
`"evidence_policy": "registration-only"`.

## Plan Change Protocol

Changing `PLAN.md` or any `plan/*.{json,md}` file requires all of: a commit
whose message starts with `plan-change:`, a new record under `plan/decisions/`
plus its `plan/decision-index.json` lifecycle entry, a `plan_version` bump, and
a `plan/repository_tree.json` update if the taxonomy moved. CI rejects any other
plan edit.

## Design Rules

- Use composition and constructor dependency injection.
- Keep ownership explicit with RAII and `std::unique_ptr` where indirection is
  necessary; owning raw pointers are forbidden.
- Do not create god structs. Separate storage, policy, execution context, and
  reporting when their lifetimes or responsibilities differ.
- Keep the C ABI opaque and stable; put C++ ergonomics in the wrapper.
- The public boundary is only `include/blitzar/c/blitzar.h` and
  `include/blitzar/cpp/blitzar.hpp`. Public headers may include only registered
  standard-library headers and the public C ABI header; MPI, HIP, CUDA, `src/`,
  and internal implementation headers are forbidden there. ABI pointer and V1
  parameter exceptions must be registered in `plan/public_boundary.json` or
  `plan/parameter_exceptions.json`.
- Native MPI headers, handles, and preprocessor symbols are confined to the
  registered units in `plan/quality.json`. `MpiContext`, domain decomposition,
  packet contracts, and ghost state consume only non-MPI descriptors; new MPI
  calls require an update to the native-boundary gate and its evidence.
- Keep one-level namespaces and never use `using namespace`.
- Do not add `utils`, `common`, `misc`, `private`, or `details` catch-alls.
- Do not split files mechanically by line count. Review responsibility,
  function count, branching, and allocation behavior together.
- Do not allocate dynamically after initialization unless the contract records
  and tests the exception. See `plan/AllocationLifecycle.md`; steady state
  after warmup must be allocation-free.
- Avoid recursion and unbounded loops in production simulation paths.

## Naming

C++, CUDA, and header files use PascalCase names that are unique by complete
filename across the code tree and match their primary type when they declare
one. A repeated stem is allowed only for an explicitly configured
implementation/header pair. Public ABI spellings and build-template names are
registered exceptions, not implicit overrides. Maximum filename lengths are
defined per repository profile in `plan/quality.json`; `short` is not a
subjective exception. Rust, Python, and configuration files use their
language's established lowercase convention. Names must not repeat the
containing component without adding meaning, and legacy/server or catch-all
prefixes and path components are forbidden.

## Validation

Every implementation phase requires deterministic tests, sanitizer or static
analysis coverage appropriate to the target, and a numerical comparison against
the reference where applicable. A feature is not complete because its files
compile; its observable behavior must be demonstrated. Source files use UTF-8
without a BOM, LF line endings, a final newline, the pinned clang-format version
from `plan/quality.json`, and the recursive grouping checker.
