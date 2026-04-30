# Qualified Prod Baseline

This artifact defines the evidence-grade baseline for qualification-oriented `prod` runs.

## Evidence-Grade Lanes

| Lane | Runner / OS | Toolchain assumptions | Required flags | Evidence status |
|---|---|---|---|---|
| `pr-fast` quality gate | `ubuntu-24.04` | hosted GNU toolchain plus `clang-tidy`, `cmake`, `ninja`, Python `3.12` tooling (`pytest`, `ruff`, `mypy`) | `-DBLITZAR_PROFILE=prod`, `-DBLITZAR_INTEGRATION_STRICT_WARNINGS=ON`, `-DBLITZAR_INTEGRATION_ENABLE_SANITIZERS=ON`, `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` | primary deterministic merge evidence |
| `nightly-full` standalone and coverage lanes | `windows-2022` plus `ubuntu-24.04` | hosted MSVC 2022 x64 and Ubuntu 24.04 GNU toolchains, `cmake`, `ninja`, Python `3.12`, coverage tooling on Linux | `-DBLITZAR_PROFILE=prod`, `-DBLITZAR_INTEGRATION_STRICT_WARNINGS=ON`, plus coverage or repeat-run flags as lane-specific extensions | extended deterministic evidence; not the merge gate |
| `release-lane` package flow | `windows-2022` | MSVC 2022 x64 via `ilammy/msvc-dev-cmd`, `cmake`, `ninja`, Python `3.12`, CUDA Toolkit `12.4.1` | `-DBLITZAR_PROFILE=prod`, `-DBLITZAR_STRICT_WARNINGS=ON` | release candidate evidence and packaging baseline |
| `release-gpu-full-tests` | self-hosted `windows/x64/cuda` | MSVC x64, `cmake`, `ninja`, Python tooling, NVIDIA runtime plus `nvcc`; readiness proven by `gpu-runner-health` | `-DBLITZAR_PROFILE=prod`, `-DBLITZAR_STRICT_WARNINGS=ON` | supplemental evidence only; never sole baseline |

## Environment Rules

- `prod` evidence requires the exact CI lane constraints above or a local reproduction that matches them explicitly.
- Hosted lane assumptions are defined by workflow files and must remain reviewable under source control.
- Hosted merge and nightly lanes must keep explicit axis labels in job names so failures map directly to runner OS and toolchain family.
- Self-hosted GPU lanes must publish a readiness report and fallback reason through `.github/workflows/gpu-runner-health.yml`.
- `pr-fast` dev module builds are intentionally `dev` profile exploratory checks and are excluded from qualification evidence.
- Dynamic client modules are never evidence by default; if `blitzar-client` is explicitly built under `prod`, only startup loading of allowlisted modules is permitted and the host must verify the sidecar manifest, product metadata, and `sha256` before loading.
- Live module `reload` / `switch` are forbidden in `prod`, even when the client host is explicitly built for local reproduction.
- Optional hardware lanes can extend evidence breadth, but merge/release qualification cannot depend on them alone.
- `dev` profile runs, ad hoc local builds, and exploratory module builds are non-evidence until reproduced under the `prod` baseline.

## Required Build and Review Controls

- Repository quality gate: `make quality-local CONFIG=simulation.ini`
- Python analysis: `make quality-python`
- Strict integration lane: `make quality-strict CONFIG=simulation.ini QUALITY_BUILD_DIR=build-quality`
- Release lane packaging: `scripts/ci/release/package_source.py`, `scripts/ci/release/package_bundle.py`, and `scripts/ci/release/package_evidence.py`

## Change Control

- Any standards-impacting change to runner family, toolchain family, CUDA expectation, or required `prod` flags must update:
  - `docs/quality/standards-profile.md`
  - `docs/quality/tool-qualification.md`
  - `docs/quality/quality_manifest.json`
- Evidence produced outside this baseline must be labeled exploratory, not qualification-grade.

