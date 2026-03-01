# Tool Qualification Baseline

This document defines minimum confidence controls for development and CI tools.

## Standards Context

- Tool confidence evidence is maintained for NASA-first assurance posture (`NPR-7150.2D`, `NASA-STD-8739.8B`).

## Tool Classes

| Tool | Role | Confidence Strategy |
|---|---|---|
| `cmake` + compiler toolchain | Build generation and compilation | pinned versions in CI lanes, strict warning policy, `GRAVITY_PROFILE=prod` in evidence lanes |
| `ctest` + `gtest` | test execution | deterministic fast subset in PR lane, broader deterministic scope in nightly/release |
| `clang-tidy` | static analyzer | analyzer checks with warnings-as-errors in strict PR lane |
| Python checks (`tests/checks/*.py`) | policy and contract guards | syntax check + mandatory execution in PR and nightly |
| GitHub Actions runners | orchestration | split deterministic lane and optional hardware lane |
| release packaging scripts | artifact assembly | reproducible scripts + review of output manifest |

## Qualification Evidence

- Workflow definitions in `.github/workflows/`.
- Reproducible check entrypoints:
  - `python tests/checks/check.py all --root . --config simulation.ini`
  - `python tests/checks/clang_tidy_check.py --root . --build-dir <build>`
- Build flags proving strict mode:
  - `GRAVITY_STRICT_WARNINGS=ON`
  - `GRAVITY_INTEGRATION_STRICT_WARNINGS=ON`
  - `GRAVITY_PROFILE=prod`

## Acceptance Rules

- Any tool change that alters diagnostics or generated artifacts must be reviewed.
- CI pipeline updates must keep at least one deterministic strict lane green.
- Optional hardware lanes cannot be the sole evidence for core requirements.
- Qualification evidence for mission-impacting changes must come from `prod` profile constraints.
