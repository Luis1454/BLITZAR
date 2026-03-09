# CUDA-GRAVITY-SIMULATION

[![nightly-full](https://github.com/Luis1454/CUDA-GRAVITY-SIMULATION/actions/workflows/nightly-full.yml/badge.svg?branch=main)](https://github.com/Luis1454/CUDA-GRAVITY-SIMULATION/actions/workflows/nightly-full.yml)

## Quick Start

```bash
make run
```

Build and test:

```bash
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

Prod profile (deterministic critical path, dynamic frontend modules disabled):

```bash
make build-prod
```

Integration tests:

```bash
make test-int
```

Repository quality gate:

```bash
make quality-local
```

Strict local preflight aligned with CI:

```bash
make quality-strict
```

`make quality-*` stays a thin wrapper around the canonical CMake/Python entrypoints used by the strict Linux gate.

## Binaries

- `myApp`
- `myAppBackend`
- `myAppHeadless`
- `myAppModuleHost`

In `PROFILE=prod`, `myAppModuleHost` and dynamic frontend modules are disabled by design.

## Coverage

- Dashboard: https://luis1454.github.io/CUDA-GRAVITY-SIMULATION/
- Workflow: `nightly-full`
- Artifact: `nightly-integration-coverage-<run_number>`

## Documentation

- Full README: [docs/README_full.md](docs/README_full.md)
- Backend protocol: [docs/backend_protocol.md](docs/backend_protocol.md)
- Frontend module host: [docs/frontend_module_host.md](docs/frontend_module_host.md)
- Quality baseline: [docs/quality/README.md](docs/quality/README.md)

## Project Layout

- `apps/`
- `engine/include/`, `engine/src/`
- `runtime/include/`, `runtime/src/`
- `modules/`
- `tests/unit`, `tests/int`: product verification
- `tests/checks`, `python_tools`: repository quality tooling and CI gates

## Config

`simulation.ini` is auto-created at first launch.

Main options are documented in [docs/README_full.md](docs/README_full.md).
