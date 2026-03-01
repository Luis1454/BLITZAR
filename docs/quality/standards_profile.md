# Standards Profile (NASA-First)

This repository is independent from any company stack but aligned for US space recruiting review (JPL and California space-tech).

## Primary Standards

- `NPR-7150.2D` (NASA Software Engineering Requirements): primary process reference.
- `NASA-STD-8739.8B` (Software Assurance and Software Safety): primary assurance reference.

## Secondary Crosswalk Standards

- `ECSS-E-ST-40C`: software engineering structure and lifecycle vocabulary.
- `ECSS-Q-ST-80C`: software product assurance structure.
- `ECSS-E-ST-40-07C`: simulation and modelling lifecycle context.

## Profile Split

- `prod` profile (qualification evidence path):
  - deterministic execution requirements apply;
  - no dynamic reload in mission-critical runtime path;
  - strict quality gate must be green in PR CI.
  - CI lanes must force `-DGRAVITY_PROFILE=prod` for qualification evidence.
- `dev` profile (iteration path):
  - broader experimentation allowed;
  - results are not qualification evidence unless reproduced in `prod` profile constraints.

Build switch:

- `-DGRAVITY_PROFILE=prod` for qualification-oriented builds.
- `-DGRAVITY_PROFILE=dev` for iteration builds.

## Mandatory Evidence Set

- Requirement catalog: `docs/quality/requirements.json`.
- Requirement traceability: `docs/quality/traceability.csv`.
- Test catalog: `docs/quality/test_catalog.csv`.
- Standards crosswalk: `docs/quality/nasa_crosswalk.csv`.
- IV&V plan: `docs/quality/ivv_plan.md`.
- Failure analysis: `docs/quality/fmea.md`.
- Numerical acceptance policy: `docs/quality/numerical_validation.md`.
- Tool confidence strategy: `docs/quality/tool_qualification.md`.
- Strict CI lane definition: `.github/workflows/pr-fast.yml`.

## Determinism Baseline

- Fixed-input regression tests are the authoritative evidence.
- PR lane executes a deterministic fast subset for merge safety.
- Nightly and release lanes execute broader deterministic scopes.
- Runtime behavior in critical paths must be reproducible under pinned toolchain settings.
- Any requirement, tolerance, or toolchain update must update the quality artifacts in this directory in the same change.
