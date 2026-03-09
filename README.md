# CUDA-GRAVITY-SIMULATION

[![nightly-full](https://github.com/Luis1454/CUDA-GRAVITY-SIMULATION/actions/workflows/nightly-full.yml/badge.svg?branch=main)](https://github.com/Luis1454/CUDA-GRAVITY-SIMULATION/actions/workflows/nightly-full.yml)

CUDA/C++ gravity simulation with a split runtime, backend, frontend modules, and repository-level quality gates aimed at deterministic, auditable workflows.

## Quickstart

Build a local Release configuration:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --no-tests=error
pytest -q tests/checks/suites/policy
```

Qualification-oriented `prod` profile:

```bash
cmake -S . -B build-prod -G Ninja -DCMAKE_BUILD_TYPE=Release -DGRAVITY_PROFILE=prod
cmake --build build-prod -j
ctest --test-dir build-prod --no-tests=error
```

## Build Entry Points

`CMake` is the source of truth for configure/build/test.

The root `Makefile` is a thin wrapper around the same commands:

- `make configure` -> `cmake -S . -B build ...`
- `make build` -> `cmake --build build --parallel`
- `make test` -> `ctest --test-dir build --no-tests=error`
- `make quality` -> `pytest -q tests/checks/suites/policy`
- `make build-prod` -> Release build with `-DGRAVITY_PROFILE=prod`
- `make check-all` -> broader Python umbrella gate via `tests/checks/check.py all`

Use `make` if you want a convenience wrapper. Use direct `cmake`/`ctest`/`pytest` commands when collecting reproducible reviewer evidence.

## Architecture

```text
apps/      -> launchers and top-level executables
engine/    -> simulation core, physics, backend-facing logic
runtime/   -> runtime services, protocols, orchestration
modules/   -> optional frontend/plugin implementations
tests/     -> C++ integration/unit tests and Python policy checks

apps -> runtime -> engine
             \
              -> modules
```

## Binaries

- `myApp`
- `myAppBackend`
- `myAppHeadless`
- `myAppModuleHost`

In `GRAVITY_PROFILE=prod`, dynamic frontend modules and `myAppModuleHost` are intentionally disabled on the deterministic path.

## Quality & Verification

- Power of 10 profile: [docs/quality/power_of_10.md](docs/quality/power_of_10.md)
- IV&V baseline: [docs/quality/ivv_plan.md](docs/quality/ivv_plan.md)
- Traceability: [docs/quality/traceability.md](docs/quality/traceability.md)
- Quality baseline overview: [docs/quality/README.md](docs/quality/README.md)

Repository policy checks live in `python_tools/checks/` and are exercised by `pytest` suites under `tests/checks/suites/policy/`.

CI authority split:

- GitHub workflows are the source of truth for repository Python gates: `check.py all`, `ruff`, `mypy`, and `pytest -q tests/checks/suites`.
- `ctest` is the source of truth for C++ and integration-safe deterministic subsets.
- `TST_QLT_REPO_008_PyChecksUnit` and `TST_QLT_REPO_009_PythonQualityGate` remain available for compatibility/manual regression, but are not part of the fast `ctest` subset.

## More Documentation

- Full project walkthrough: [docs/README_full.md](docs/README_full.md)
- Backend protocol: [docs/backend_protocol.md](docs/backend_protocol.md)
- Frontend module host: [docs/frontend_module_host.md](docs/frontend_module_host.md)
