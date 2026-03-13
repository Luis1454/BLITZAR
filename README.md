# A.S.T.E.R.

Accelerated Simulation for Trajectory and Energy Resolution.

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

Prod profile (deterministic critical path, dynamic client modules disabled):

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

- `aster`
- `aster-server`
- `aster-headless`
- `aster-client`

In `PROFILE=prod`, `aster-client` and dynamic client modules are disabled by design. In `PROFILE=dev`, client modules load through a manifest-verified, checksum-checked host path.

## Coverage

- Dashboard: https://luis1454.github.io/CUDA-GRAVITY-SIMULATION/
- Workflow: `nightly-full`
- Artifact: `nightly-integration-coverage-<run_number>`

## Documentation

- Full README: [docs/README_full.md](docs/README_full.md)
- Server protocol: [docs/server_protocol.md](docs/server_protocol.md)
- Client host: [docs/client_host.md](docs/client_host.md)
- Quality baseline: [docs/quality/README.md](docs/quality/README.md)

## Project Layout

- `apps/`
- `engine/include/`, `engine/src/`
- `runtime/include/`, `runtime/src/`
- `modules/`
- `tests/unit`, `tests/int`: product verification
- `tests/checks`, `python_tools`: repository quality tooling and CI gates

## Config

`simulation.ini` is auto-created at first launch and now uses directive blocks such as `simulation(...)`, `scene(...)`, `thermal(...)`, and `client(...)`.

Main options are documented in [docs/README_full.md](docs/README_full.md).
