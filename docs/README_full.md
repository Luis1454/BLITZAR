# CUDA-GRAVITY-SIMULATION

[![nightly-full](https://github.com/Luis1454/CUDA-GRAVITY-SIMULATION/actions/workflows/nightly-full.yml/badge.svg?branch=main)](https://github.com/Luis1454/CUDA-GRAVITY-SIMULATION/actions/workflows/nightly-full.yml)

## Coverage Dashboard
| Metric | Live Percent |
|---|---|
| Lines | ![Coverage lines](https://img.shields.io/endpoint?url=https%3A%2F%2Fluis1454.github.io%2FCUDA-GRAVITY-SIMULATION%2Fcoverage%2Flines.json) |
| Functions | ![Coverage functions](https://img.shields.io/endpoint?url=https%3A%2F%2Fluis1454.github.io%2FCUDA-GRAVITY-SIMULATION%2Fcoverage%2Ffunctions.json) |
| Branches | ![Coverage branches](https://img.shields.io/endpoint?url=https%3A%2F%2Fluis1454.github.io%2FCUDA-GRAVITY-SIMULATION%2Fcoverage%2Fbranches.json) |

Coverage payload is published by `nightly-full` to GitHub Pages (`coverage/*.json`).

## Build

Quick start:

```bash
make run
```

Helper:

```bash
make help
```

Direct CMake:

```bash
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

Profiles:

```bash
make build-dev
make build-run
make build-ci
make build-prod
```

Standalone integration suite:

```bash
make test-int
```

Built binaries:
- `myApp` (launcher)
- `myAppServer` (server daemon)
- `myAppHeadless` (headless simulation)
- `myAppClient` (+ dynamic client modules in `dev` profile)

## CI Lanes

- `pr-fast`: merge gate (repo checks, strict deterministic subset, analyzer).
- `nightly-full`: broader deterministic scopes + coverage publishing + optional GPU lane.
- `release-lane`: release-oriented deterministic lane + optional GPU lane.

Evidence lanes force `GRAVITY_PROFILE=prod` and use `ctest --no-tests=error`.

## Project Layout

- `apps/`: entrypoints (`launcher`, `server-service`, `headless`, `client-host`)
- `engine/include/`, `engine/src/`: core server/config/physics/platform
- `runtime/include/`, `runtime/src/`: runtime protocol/client/server wiring
- `modules/`: pluggable clients (`cli`, `echo`, `proxy`, `qt`)
- `tests/`: unit, integration, support harnesses, repository quality checks
- `docs/quality/`: NASA-first quality baseline artifacts

Key files:
- `apps/launcher/main.cpp`
- `apps/server-service/main.cpp`
- `apps/headless/main.cu`
- `apps/client-host/main.cpp`
- `engine/src/physics/cuda/ParticleSystem.cu`
- `modules/qt/ui/MainWindow.cpp`

## Configuration

`simulation.ini` is created automatically on first launch.

Core keys:
- `particle_count`
- `dt`
- `solver` (`pairwise_cuda`, `octree_gpu`, `octree_cpu`)
- `integrator` (`euler`, `rk4`)
- `sph_enabled` and SPH coefficients
- `octree_theta`, `octree_softening`
- `client_remote_*_timeout_ms`
- `export_directory`, `export_format`
- `input_file`, `input_format`
- `init_*` initial-state parameters
- `energy_measure_every_steps`, `energy_sample_limit`

For the complete schema and behavior contract, see:
- `engine/src/config/SimulationConfig.cpp`
- `engine/src/config/SimulationArgs.cpp`

## Run Modes

Launcher modes:

```bash
build/myApp.exe --mode client -- --config simulation.ini --module qt
build/myApp.exe --mode server -- --config simulation.ini --server-host 127.0.0.1 --server-port 4545
build/myApp.exe --mode headless -- --config simulation.ini --particle-count 50000 --target-steps 1000
```

Direct binaries:

```bash
build/myAppServer.exe --config simulation.ini --server-host 127.0.0.1 --server-port 4545
build/myAppClient.exe --config simulation.ini --module cli
build/myAppHeadless.exe --config simulation.ini --particle-count 50000 --target-steps 1000
```

Module host runtime switch:

```text
switch cli
switch echo
switch gui
switch qt
```

## Export / Import

Supported snapshot formats:
- `vtk`
- `vtk_binary`
- `xyz`
- `bin`

Import is controlled by:

```ini
init_mode=file
input_file=exports/sim_20260217_122420_s41.vtk
input_format=auto
```

If `init_mode` is not `file`, initial state is generated from `init_*` parameters.

## Quality Baseline

NASA-first evidence lives in `docs/quality/`:
- `quality_manifest.json`
- `standards_profile.md`
- `ivv_plan.md`
- `fmea.md`
- `tool_qualification.md`
- `numerical_validation.md`

Repository contract checks:

```bash
make quality-local
make quality-strict
```
