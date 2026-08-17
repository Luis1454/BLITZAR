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
make test
```

`make test` selects the platform developer preset, builds the product, then configures,
builds, and runs the deterministic `integration-safe` test preset. Use the CMake commands
directly when selecting an explicit profile.

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

## Reproducible Development

Docker is the recommended fallback when a local C++ toolchain is unavailable. The CPU image installs
GCC, CMake and Ninja, compiles the headless runtime path, then exposes the headless binary as its
runtime entry point. Integration tests remain available through the `integration-safe` CMake
preset and CI quality lanes:

```bash
docker build -f Dockerfile.cpu -t blitzar-cpu:local .
docker run --rm blitzar-cpu:local
docker run --rm blitzar-cpu:local --validate --config /blitzar/simulation.ini
docker run --rm -v "$(pwd)/outputs:/blitzar/outputs" blitzar-cpu:local \
  --run --config /blitzar/simulation.ini --solver octree_cpu \
  --target-steps 100 --deterministic true \
  --export-on-exit true --export-path /blitzar/outputs/final.xyz
```

The default container command is `--inspect`: it loads and prints the resolved configuration without
starting the solver. `--validate` performs the same load and refuses invalid cases. A calculation is
only started by `--run`; `--export-path` makes the output name reproducible. The container validates compilation and headless execution. It does not replace native Windows
validation of the console-free GUI, MSVC ABI, CUDA kernels or the NSIS installer.

Headless workflow:

```text
case/simulation.ini -> --inspect -> --validate -> --run -> case/output/final.xyz
```

For a complete case directory with native or Docker execution, logs, provenance manifests, and output
hashes, use [docs/case-workflow.md](docs/case-workflow.md) and `scripts/blitzar_case.py`.

The headless executable reports the absolute configuration path, the selected simulation profile,
the generated or file-backed initial state, the effective configuration, the solver, and the output
path before calculating. Relative input and output paths are resolved from the configuration file
directory.

On Windows, use a Visual Studio Developer PowerShell with Qt6 and run:

```powershell
cmake --preset windows-desktop -DCMAKE_PREFIX_PATH="C:/Qt/6.8.2/msvc2022_64"
cmake --build --preset windows-desktop --target blitzar-gui blitzar-client blitzar-server blitzarClientModuleQtInProc --parallel
```

On macOS, install Homebrew dependencies and use the dedicated CPU profile:

```bash
brew install cmake ninja libomp qt
cmake --preset macos-dev -DCMAKE_PREFIX_PATH="$(brew --prefix libomp);$(brew --prefix qt)"
cmake --build --preset macos-dev --target blitzar-gui blitzar-headless blitzar-server blitzar-client blitzarClientModuleQtInProc --parallel
```

The canonical explicit profiles are `linux-dev`, `macos-dev`, `linux-prod`,
`macos-prod`, `windows-desktop`, `cuda-runtime`, and `release-prod`. Integration and quality profiles live in
`tests/CMakePresets.json` and are invoked by `make test` or `make quality-strict`.

## Binaries

- `blitzar`
- `blitzar-gui`
- `blitzar-server`
- `blitzar-headless`
- `blitzar-client`

In `PROFILE=prod`, `blitzar-client` and dynamic client modules are disabled by design. In `PROFILE=dev`, client modules load through a manifest-verified, checksum-checked host path.

## Portable Windows Bundle

The release lane packages a tracked source archive in `dist/source/` and a zipped Windows runtime bundle in `dist/release-bundle/`. The runtime bundle always includes the built BLITZAR executables plus `simulation.ini`, `README.md`, and `tool_manifest.json` when available. When a Windows build also contains client modules or Qt runtime files, the bundle now preserves the required adjacent `.dll`, `.dll.manifest`, and Qt plugin directories such as `platforms/qwindows.dll`.

The same lane also publishes a separate desktop GUI installer executable named `blitzar-<tag>-windows-desktop-installer.exe`. This is a convenience `dev`-profile desktop package, not the qualification evidence artifact. Run it to install under `%LOCALAPPDATA%\Programs\BLITZAR` with Start Menu and desktop shortcuts. Those shortcuts launch the console-free `blitzar-gui.exe`, which starts the Qt workspace and keeps the host alive until the GUI window exits.

On `v*` tags, or manual dispatch with a `v*` release tag, the lane publishes a GitHub Release with the source archive, executable bundle, SBOM, evidence pack, and release-quality index. It also extracts the generated runtime archive and smoke-validates the portable layout on a clean hosted Windows runner by executing the packaged help commands for each bundled executable.

## Documentation

- Server protocol: [docs/server-protocol.md](docs/server-protocol.md)
- Client host: [docs/client-host.md](docs/client-host.md)
- Quality baseline: [docs/quality/quality-overview.md](docs/quality/quality-overview.md)
- Operational control: [docs/quality/operational-control.md](docs/quality/operational-control.md)

## Project Layout

- `apps/`
- `engine/<domain>/<module>/include`, `src`, `cuda`, `tests`, `Module.cmake`
- `runtime/`, `runtime/`
- `modules/`
- `tests/unit`, `tests/int`: product verification
- `tests/checks`, `python_tools`: repository quality tooling and CI gates

## Config

`simulation.ini` is auto-created at first launch and now uses directive blocks such as `simulation(...)`, `performance(...)`, `scene(...)`, `thermal(...)`, and `client(...)`.

Main options are available from the executable help output and mirrored by the option registry in `engine/config/SimulationOptionRegistryEntries.cpp`.
