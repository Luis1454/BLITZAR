# A.S.T.E.R.

Accelerated Simulation for Trajectory and Energy Resolution.

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
- `aster` (launcher)
- `aster-server` (server daemon)
- `aster-headless` (headless simulation)
- `aster-client` (+ manifest-verified dynamic client modules in `dev` profile)

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

For the complete schema and behavior contract, see:
- `engine/src/config/SimulationConfig.cpp`
- `engine/src/config/SimulationArgs.cpp`

## Run Modes

Launcher modes:

```bash
build/aster.exe --mode client -- --config simulation.ini --module qt
build/aster.exe --mode server -- --config simulation.ini --server-host 127.0.0.1 --server-port 4545
build/aster.exe --mode headless -- --config simulation.ini --particle-count 50000 --target-steps 1000
```

Direct binaries:

```bash
build/aster-server.exe --config simulation.ini --server-host 127.0.0.1 --server-port 4545
build/aster-client.exe --config simulation.ini --module cli
build/aster-headless.exe --config simulation.ini --particle-count 50000 --target-steps 1000
```

`aster-client` is a `dev`-profile path. Each client module now ships with a sidecar manifest and the host verifies the module allowlist, `api_version`, product metadata, and `sha256` digest before loading it. If the client host is explicitly enabled under `prod`, startup load stays manifest-verified but live `reload` / `switch` are disabled.

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
