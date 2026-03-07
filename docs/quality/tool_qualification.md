# Tool Qualification Baseline

This document defines minimum confidence controls for development and CI tools.

## Standards Context

- Tool confidence evidence is maintained for NASA-first assurance posture (`NPR-7150.2D`, `NASA-STD-8739.8B`).

## Tool Classes

| Tool | Role | Confidence Strategy |
|---|---|---|
| `cmake` + compiler toolchain | Build generation and compilation | pinned versions in CI lanes, strict warning policy, `GRAVITY_PROFILE=prod` in evidence lanes |
| `ctest` + `gtest` | test execution | deterministic fast subset in `pr-fast`, broader deterministic scope in `nightly-full`, release packaging validation in `release-lane` |
| `clang-tidy` | static analyzer | analyzer checks with warnings-as-errors in strict PR lane |
| Python checks (`tests/checks/*.py`) | policy and contract guards | syntax check + mandatory execution in PR and nightly |
| GitHub Actions runners | orchestration | split merge gate, extended nightly evidence lanes, release packaging lane, and optional hardware lanes |
| release packaging scripts | artifact assembly | reproducible scripts + review of output manifest |

## Qualification Evidence

- Workflow definitions in `.github/workflows/`.
- Qualified environment baseline in `docs/quality/prod_baseline.md`.
- Generated tool manifest format in `docs/quality/tool_manifest.md`.
- Reproducible check entrypoints:
  - `python tests/checks/check.py all --root . --config simulation.ini`
  - `python tests/checks/clang_tidy_check.py --root . --build-dir <build>`
  - `python scripts/ci/release/package_tool_manifest.py --lane <lane> --profile prod`
- Build flags proving strict mode:
  - `GRAVITY_STRICT_WARNINGS=ON`
  - `GRAVITY_INTEGRATION_STRICT_WARNINGS=ON`
  - `GRAVITY_PROFILE=prod`

## Toolchain Review Checklist

- Review compiler, CMake, Python, `clang-tidy`, and runner OS version changes from the generated manifest.
- Confirm the affected CI lane still matches `docs/quality/prod_baseline.md`.
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
- Changes to the qualified `prod` environment must be synchronized with `prod_baseline.md`.
