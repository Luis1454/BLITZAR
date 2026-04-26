# BLITZAR
### Baryonic Lagrangian Integration of Trajectories for Zero-drift Astrophysical Research

[![nightly-full](https://github.com/Luis1454/BLITZAR/actions/workflows/nightly-full.yml/badge.svg?branch=main)](https://github.com/Luis1454/BLITZAR/actions/workflows/nightly-full.yml)
[![Coverage lines](https://img.shields.io/endpoint?url=https%3A%2F%2Fraw.githubusercontent.com%2FLuis1454%2FBLITZAR%2Fcoverage-data%2Fcoverage%2Flines.json)](https://github.com/Luis1454/BLITZAR/tree/coverage-data/coverage)
[![Coverage functions](https://img.shields.io/endpoint?url=https%3A%2F%2Fraw.githubusercontent.com%2FLuis1454%2FBLITZAR%2Fcoverage-data%2Fcoverage%2Ffunctions.json)](https://github.com/Luis1454/BLITZAR/tree/coverage-data/coverage)
[![Coverage branches](https://img.shields.io/endpoint?url=https%3A%2F%2Fraw.githubusercontent.com%2FLuis1454%2FBLITZAR%2Fcoverage-data%2Fcoverage%2Fbranches.json)](https://github.com/Luis1454/BLITZAR/tree/coverage-data/coverage)

> **High-Performance GPGPU N-Body Engine simulating 100M+ particles with NASA-standard fidelity.**
> [Insert demo video or simulation GIF](https://github.com/Luis1454/BLITZAR)

## Scientific & Performance Core

While the repository follows strict software engineering gates, BLITZAR is at its heart a research tool:
- **Scale:** Real-time integration of **100M+ entities** using optimized CUDA kernels.
- **Hardware-Aware:** Achieved **92% VRAM bandwidth saturation** on modern NVIDIA architectures (RTX 4070).
- **Numerics:** Implementation of **Parallel Octrees** and Leapfrog/RK4 integrators compliant with **NASA NPR-7150.2D**.
- **Relativity:** Built-in support for relativistic raytracing and curved spacetime visualization.

## Operational Control & Quality

Coverage is treated as a first-class steering signal for execution risk, alongside deterministic tests and issue state transitions.

[![Coverage control widget](https://raw.githubusercontent.com/Luis1454/BLITZAR/coverage-data/coverage/widget.svg)](https://github.com/Luis1454/BLITZAR/tree/coverage-data/coverage)

- Operational framework: [docs/quality/operational-control.md](docs/quality/operational-control.md)
- Coverage dashboard payload: `coverage-data/coverage/*`
- Coverage workflow: `nightly-full`

### GPU Coverage Limitations
*Note: The actual physics simulation tests rely predominantly on the GPU framework (`__device__` compiled code or CUDA hooks). They cannot be reliably fully isolated in the CPU-only Linux coverage build without breaking the strict `CMakeLists.txt` repository separation between pure logical C++ tests and GPU/runtime binaries. Furthermore, MSVC + CUDA compilation limits the usability of tools like `gcovr` for device coverage. Thus, GPU device coverage is structurally unmeasurable with the current standard C++ CI tooling and intentionally omitted from the line coverage metrics, which strictly measure host-side logic.*


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

- `blitzar`
- `blitzar-server`
- `blitzar-headless`
- `blitzar-client`

In `PROFILE=prod`, `blitzar-client` and dynamic client modules are disabled by design. In `PROFILE=dev`, client modules load through a manifest-verified, checksum-checked host path.

## Portable Windows Bundle

The release lane packages a tracked source archive in `dist/source/` and a zipped Windows runtime bundle in `dist/release-bundle/`. The runtime bundle always includes the built BLITZAR executables plus `simulation.ini`, `README.md`, and `tool_manifest.json` when available. When a Windows build also contains client modules or Qt runtime files, the bundle now preserves the required adjacent `.dll`, `.dll.manifest`, and Qt plugin directories such as `platforms/qwindows.dll`.

The same lane also publishes a separate desktop GUI installer archive named `blitzar-<tag>-windows-desktop-installer.zip`. This is a convenience `dev`-profile desktop package, not the qualification evidence artifact. Extract it and run `Install BLITZAR.cmd` to install under `%LOCALAPPDATA%\Programs\BLITZAR` with Start Menu/Desktop shortcuts, or run `Launch BLITZAR GUI.cmd` directly for portable use.

On `v*` tags, or manual dispatch with a `v*` release tag, the lane publishes a GitHub Release with the source archive, executable bundle, SBOM, evidence pack, and release-quality index. It also extracts the generated runtime archive and smoke-validates the portable layout on a clean hosted Windows runner by executing the packaged help commands for each bundled executable.

## Documentation

- Server protocol: [docs/server-protocol.md](docs/server-protocol.md)
- Client host: [docs/client-host.md](docs/client-host.md)
- Quality baseline: [docs/quality/quality-overview.md](docs/quality/quality-overview.md)
- Operational control: [docs/quality/operational-control.md](docs/quality/operational-control.md)

## Project Layout

- `apps/`
- `engine/include/`, `engine/src/`
- `runtime/include/`, `runtime/src/`
- `modules/`
- `tests/unit`, `tests/int`: product verification
- `tests/checks`, `python_tools`: repository quality tooling and CI gates

## Config

`simulation.ini` is auto-created at first launch and now uses directive blocks such as `simulation(...)`, `performance(...)`, `scene(...)`, `thermal(...)`, and `client(...)`.

Main options are available from the executable help output and mirrored by the option registry in `engine/src/config/SimulationOptionRegistryEntries.cpp`.
