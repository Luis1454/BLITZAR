# BLITZAR

Baryonic Lagrangian Integration for Trajectories & Zero-drift Astrophysical Resolution

[![nightly-full](https://github.com/Luis1454/BLITZAR/actions/workflows/nightly-full.yml/badge.svg?branch=main)](https://github.com/Luis1454/BLITZAR/actions/workflows/nightly-full.yml)

## Coverage Dashboard
[![Coverage control widget](https://raw.githubusercontent.com/Luis1454/BLITZAR/coverage-data/coverage/widget.svg)](https://github.com/Luis1454/BLITZAR/tree/coverage-data/coverage)

| Metric | Live Percent |
|---|---|
| Lines | ![Coverage lines](https://img.shields.io/endpoint?url=https%3A%2F%2Fraw.githubusercontent.com%2FLuis1454%2FBLITZAR%2Fcoverage-data%2Fcoverage%2Flines.json) |
| Functions | ![Coverage functions](https://img.shields.io/endpoint?url=https%3A%2F%2Fraw.githubusercontent.com%2FLuis1454%2FBLITZAR%2Fcoverage-data%2Fcoverage%2Ffunctions.json) |
| Branches | ![Coverage branches](https://img.shields.io/endpoint?url=https%3A%2F%2Fraw.githubusercontent.com%2FLuis1454%2FBLITZAR%2Fcoverage-data%2Fcoverage%2Fbranches.json) |

Coverage payload is published by `nightly-full` to the dedicated `coverage-data` branch (`coverage/*.json`, `coverage/widget.svg`).

Operational steering rules for using coverage as a central signal are documented in [docs/quality/operational_control.md](quality/operational_control.md).

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
- `blitzar` (launcher)
- `blitzar-server` (server daemon)
- `blitzar-headless` (headless simulation)
- `blitzar-client` (+ manifest-verified dynamic client modules in `dev` profile)

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

Primary directives:
- `simulation(...)`
- `performance(...)`
- `octree(...)`
- `client(...)`
- `export(...)`
- `scene(...)`
- `preset(...)`
- `thermal(...)`
- `generation(...)`
- `central_body(...)`
- `disk(...)`
- `cloud(...)`
- `sph(...)`

Example:

```ini
simulation(particle_count=10000, dt=0.01, solver=octree_gpu, integrator=euler)
performance(profile=interactive)
scene(style=preset, preset=two_body, mode=two_body, file="", format=auto)
preset(size=6, velocity_temperature=0, temperature=0)
thermal(ambient=0, specific_heat=1, heating=0, radiation=0)
client(zoom=8, luminosity=100, ui_fps=60, command_timeout_ms=80, status_timeout_ms=40, snapshot_timeout_ms=140)
```

`performance(profile=interactive|balanced|quality)` controls the main interactive budget:
- published snapshot cadence
- visible draw cap
- energy sampling cadence
- bounded substep policy

If you need explicit overrides, save/load emits `performance(profile=custom, ...)`.

Legacy flat `key=value` files remain readable for migration, but new saves are emitted in directive form.

Unit convention:
- all physical quantities use SI by default
- time: `[s]`
- distance / softening / extents: `[m]`
- mass: `[kg]`
- velocity: `[m/s]`
- acceleration: `[m/s^2]`
- energy: `[J]`
- temperature: `[K]`
- fields explicitly suffixed with `_ms` remain in milliseconds

For the complete schema and behavior contract, see:
- `engine/src/config/SimulationConfig.cpp`
- `engine/src/config/SimulationArgs.cpp`

## Run Modes

Launcher modes:

```bash
build/blitzar.exe --mode client -- --config simulation.ini --module qt
build/blitzar.exe --mode server -- --config simulation.ini --server-host 127.0.0.1 --server-port 4545
build/blitzar.exe --mode headless -- --config simulation.ini --particle-count 50000 --target-steps 1000
```

Direct binaries:

```bash
build/blitzar-server.exe --config simulation.ini --server-host 127.0.0.1 --server-port 4545
build/blitzar-client.exe --config simulation.ini --module cli
build/blitzar-headless.exe --config simulation.ini --particle-count 50000 --target-steps 1000
```

On Windows, building `gravityClientModuleQtInProc` now runs `windeployqt` automatically when the tool is available, so `platforms/qwindows.dll` and the required Qt DLLs land in the build directory next to `blitzar-client.exe`.

The release lane packages the resulting Windows runtime layout as a portable ZIP bundle. When Qt/client-module artifacts are present in the build directory, the bundle keeps the adjacent `.dll`, `.dll.manifest`, and plugin directories, then extracts the archive and smoke-runs the packaged executables on the hosted Windows runner.

Manual fallback if needed:

```bash
make deploy-qt BUILD_DIR=build-dev
```

`blitzar-client` is a `dev`-profile path. Each client module now ships with a sidecar manifest and the host verifies the module allowlist, `api_version`, product metadata, and `sha256` digest before loading it. If the client host is explicitly enabled under `prod`, startup load stays manifest-verified but live `reload` / `switch` are disabled.

Module host runtime switch (`dev` only):

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
scene(style=preset, preset=file, mode=file, file=exports/sim_20260217_122420_s41.vtk, format=auto)
```

If the resolved scene kind is not `file`, initial state is generated from the grouped scene directives.

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
