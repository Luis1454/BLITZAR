# Qualified Prod Baseline

This artifact defines the evidence-grade baseline for qualification-oriented `prod` runs.

## Evidence-Grade Lanes

| Lane | Runner / OS | Toolchain assumptions | Required flags | Evidence status |
|---|---|---|---|---|
| `pr-fast` quality gate | `ubuntu-latest` | hosted compiler toolchain, `cmake`, `ninja`, `clang-tidy`, Python tooling (`pytest`, `ruff`, `mypy`) | `-DGRAVITY_PROFILE=prod`, `-DGRAVITY_INTEGRATION_STRICT_WARNINGS=ON`, `-DGRAVITY_INTEGRATION_ENABLE_SANITIZERS=ON`, `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` | primary deterministic merge evidence |
| `nightly-full` standalone and coverage lanes | `windows-latest` plus `ubuntu-latest` | hosted MSVC and Linux compiler toolchains, `cmake`, `ninja`, Python tooling, coverage tooling on Linux | `-DGRAVITY_PROFILE=prod`, `-DGRAVITY_INTEGRATION_STRICT_WARNINGS=ON`, plus coverage or repeat-run flags as lane-specific extensions | extended deterministic evidence; not the merge gate |
| `release-lane` package flow | `windows-latest` | MSVC x64 via `ilammy/msvc-dev-cmd`, `cmake`, `ninja`, Python tooling, CUDA Toolkit `12.4.1` | `-DGRAVITY_PROFILE=prod`, `-DGRAVITY_STRICT_WARNINGS=ON` | release candidate evidence and packaging baseline |
| `release-gpu-full-tests` | self-hosted `windows/x64/cuda` | MSVC x64, `cmake`, `ninja`, Python tooling, NVIDIA runtime plus `nvcc` | `-DGRAVITY_PROFILE=prod`, `-DGRAVITY_STRICT_WARNINGS=ON` | supplemental evidence only; never sole baseline |

## Environment Rules

- `prod` evidence requires the exact CI lane constraints above or a local reproduction that matches them explicitly.
- Hosted lane assumptions are defined by workflow files and must remain reviewable under source control.
- `pr-fast` dev module builds are intentionally `dev` profile exploratory checks and are excluded from qualification evidence.
- Optional hardware lanes can extend evidence breadth, but merge/release qualification cannot depend on them alone.
- `dev` profile runs, ad hoc local builds, and exploratory module builds are non-evidence until reproduced under the `prod` baseline.

## Required Build and Review Controls

- Repository quality gate: `python tests/checks/check.py all --root . --config simulation.ini`
- Python analysis: `python -m ruff check .` and `python -m mypy tests/checks scripts/ci python_tools`
- Strict integration lane: `tests/checks/run.py clang_tidy` plus deterministic `ctest` subset under sanitizer-enabled `prod` flags
- Release lane packaging: `scripts/ci/release/package_bundle.py` and `scripts/ci/release/package_evidence.py`

## Change Control

- Any standards-impacting change to runner family, toolchain family, CUDA expectation, or required `prod` flags must update:
  - `docs/quality/standards_profile.md`
  - `docs/quality/tool_qualification.md`
  - `docs/quality/quality_manifest.json`
- Evidence produced outside this baseline must be labeled exploratory, not qualification-grade.
