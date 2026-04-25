# Tool Qualification Baseline

This document defines minimum confidence controls for development and CI tools.

## Standards Context

- Tool confidence evidence is maintained for NASA-first assurance posture (`NPR-7150.2D`, `NASA-STD-8739.8B`).

## Tool Classes

| Tool | Role | Confidence Strategy |
|---|---|---|
| `cmake` + compiler toolchain | Build generation and compilation | pinned runner images in CI lanes (`ubuntu-24.04`, `windows-2022`), strict warning policy, `GRAVITY_PROFILE=prod` in evidence lanes |
| `ctest` + `gtest` | test execution | deterministic fast subset in `pr-fast`, broader deterministic scope in `nightly-full`, release packaging validation in `release-lane` |
| `clang-tidy` | static analyzer | analyzer checks with warnings-as-errors in strict PR lane |
| `rustc` + `cargo` | host-side protocol/runtime crates, bridge-state FFI support, and optional web gateway adapter | pinned by `rust-toolchain.toml`, `Cargo.lock` committed, `cargo fmt --check` plus `cargo test` required in strict local preflight, not yet part of qualified `prod` evidence lanes |
| `windeployqt` | Windows Qt runtime deployment for the Qt client module | discovered from the pinned Qt installation, exercised on Qt module builds, and checked by `scripts/doctor.cmake` when Windows GUI artifacts are present |
| Python checks (`tests/checks/check.py`, `tests/checks/run.py`, `tests/checks/catalog.json`) | policy and contract guards | syntax check + mandatory execution in PR and nightly |
| GitHub Actions runners | orchestration | split merge gate, extended nightly evidence lanes, release packaging lane, optional hardware lanes, and explicit axis labels in hosted job names |
| self-hosted GPU runner automation | optional CUDA execution | scripted bootstrap plan, daily readiness report, and hosted fallback job summaries |
| release packaging scripts | artifact assembly | reproducible scripts + review of output manifest |

## clang-tidy Analyzer Profile

### Active Checks

The CI analyzer profile is: `-*,clang-analyzer-*,bugprone-unused-return-value`.

| Check group | Purpose |
|---|---|
| `clang-analyzer-*` | Clang static analyzer suite: null-pointer dereference, use-after-free, uninitialized reads, dead stores, logic errors. Core safety net for C++ memory and control-flow correctness |
| `bugprone-unused-return-value` | Detects ignored return values from status-returning APIs. Complements `[[nodiscard]]` annotations on internal status types |

### Deliberately Excluded

All other `clang-tidy` check families (`modernize-*`, `readability-*`, `performance-*`, `cppcoreguidelines-*`, `misc-*`, `cert-*`) are disabled (`-*` prefix). Rationale:

- **Noise risk**: enabling broad style/modernize checks on a codebase with legacy patterns produces high-volume warnings that dilute safety-critical findings and slow the CI gate.
- **Incremental widening policy**: additional check families may be enabled only when (1) file decomposition issues #288 and #289 are closed, reducing per-file complexity, and (2) the resulting warnings can be resolved within one sprint without destabilizing the physics engine.

### CUDA Device Code Exclusion

CUDA device code (`.cu` files, `__global__`/`__device__` kernels) is structurally excluded from clang-tidy analysis:

- **Compile database**: on the Linux CI runner (no CUDA toolkit), `ParticleSystem.cu` is not compiled and therefore absent from `compile_commands.json`.
- **Path scope**: `DEFAULT_PATHS` in the Python runner covers `engine/src/config/`, `runtime/src/`, and `tests/` — not `engine/src/physics/cuda/`.
- **Language support**: clang-tidy cannot parse CUDA-specific syntax (`<<<...>>>`, `__shared__`, `__syncthreads()`) without `--cuda-gpu-arch` and a CUDA-aware compilation database.

This exclusion is permanent with the current CI architecture. Mitigating it would require a self-hosted Linux runner with the CUDA toolkit installed and a dedicated analysis step using a CUDA-aware `compile_commands.json`. Until such a runner is operational, CUDA device code quality is verified through deterministic simulation tests (`TST_UNT_PHYS_*`, `TST_INT_PROT_011`) rather than static analysis.

## Qualification Evidence

- Workflow definitions in `.github/workflows/`.
- Qualified environment baseline in `docs/quality/production-baseline.md`.
- Self-hosted GPU runner operations note in `docs/quality/gpu-runner-operations.md`.
- Generated tool manifest format in `docs/quality/tool-manifest.md`.
- Reproducible check entrypoints:
  - `make quality-local CONFIG=simulation.ini`
  - `make quality-python`
  - `make quality-analyze QUALITY_BUILD_DIR=<build>`
  - `python scripts/ci/release/package_tool_manifest.py --lane <lane> --profile prod`
- Build flags proving strict mode:
  - `GRAVITY_STRICT_WARNINGS=ON`
  - `GRAVITY_INTEGRATION_STRICT_WARNINGS=ON`
  - `GRAVITY_PROFILE=prod`
- Rust workspace pinning:
  - `rust-toolchain.toml`
  - `rust/Cargo.lock`
- Current Rust-covered interfaces:
  - `rust/blitzar-protocol`
  - `rust/blitzar-runtime`
  - `rust/blitzar-web-gateway`
  - `runtime/include/ffi/BlitzarRuntimeBridgeApi.hpp`

## Toolchain Review Checklist

- Review compiler, CMake, Python, `clang-tidy`, and runner OS version changes from the generated manifest.
- Review `rust-toolchain.toml`, `Cargo.lock`, crate additions, and exported FFI symbols when the Rust workspace changes.
- Confirm hosted evidence lanes still pin `ubuntu-24.04` or `windows-2022`, Python `3.12`, and CUDA `12.4.1` where the release lane requires it.
- Confirm the affected CI lane still matches `docs/quality/production-baseline.md`.
- Confirm release bundle and release review artifacts still carry `tool_manifest.json` with CI run references.
- Treat any tool change that can alter diagnostics, ABI, generated files, or analyzer behavior as standards-impacting.

## Retrieval

- PR lane: download the `tool-qualification-pr-fast-*` artifact from `pr-fast`.
- Nightly lane: review `nightly-full` logs, coverage, and optional GPU artifacts as extended evidence, not merge-gate evidence.
- Release lane: download the `tool-qualification-release-*` artifact or inspect `tool_manifest.json` inside the release bundle.
- CI run references are stored in the manifest under `ci_context`.

## Acceptance Rules

- Any tool change that alters diagnostics or generated artifacts must be reviewed.
- CI pipeline updates must keep the strict merge gate green and preserve the documented separation between merge, nightly, and release evidence scopes.
- Optional hardware lanes cannot be the sole evidence for core requirements.
- Qualification evidence for mission-impacting changes must come from `prod` profile constraints.
- Changes to the qualified `prod` environment must be synchronized with `production-baseline.md`.

