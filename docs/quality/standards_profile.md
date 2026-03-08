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
  - strict PR quality gate must be green before merge.
  - CI lanes must force `-DGRAVITY_PROFILE=prod` for qualification evidence.
  - repository policy checks reject evidence workflow configure commands that omit `-DGRAVITY_PROFILE=prod`.
  - evidence-grade environment assumptions are fixed by `docs/quality/prod_baseline.md`.
- `dev` profile (iteration path):
  - broader experimentation allowed;
  - results are not qualification evidence unless reproduced in `prod` profile constraints.

Build switch:

- `-DGRAVITY_PROFILE=prod` for qualification-oriented builds.
- `-DGRAVITY_PROFILE=dev` for iteration builds.

## Mandatory Evidence Set

- Contribution workflow artifact: `AGENTS.md`.
- Canonical quality manifest: `docs/quality/quality_manifest.json`.
- IV&V plan: `docs/quality/ivv_plan.md`.
- Failure analysis: `docs/quality/fmea.md`.
- Numerical acceptance policy: `docs/quality/numerical_validation.md`.
- Tool confidence strategy: `docs/quality/tool_qualification.md`.
- Power of 10 coding profile: `docs/quality/power_of_10.md`.
- Qualified `prod` baseline: `docs/quality/prod_baseline.md`.
- Critical interface contracts: `docs/quality/interface_contracts.md`.
- Deviation register: `docs/quality/manifest/deviations.json`.
- Release quality index format: `docs/quality/release_index.md`.
- Strict merge gate definition: `.github/workflows/pr-fast.yml` and `.github/workflows/pr-fast-quality-gate.yml`.
- Extended deterministic evidence lanes: `.github/workflows/nightly-full.yml` and `.github/workflows/release-lane.yml`.
- Release evidence pack format: `docs/quality/evidence_pack.md`.

## Determinism Baseline

- Fixed-input regression tests are the authoritative evidence.
- `pr-fast` executes repository policy gates, analyzer checks, and a deterministic fast subset for merge safety.
- `main` must only receive commits that are traceable to merged `issue/<N>-<slug>` pull requests.
- `nightly-full` extends deterministic evidence with repeated standalone integration runs, coverage publication, FMEA status snapshots, and optional GPU full-suite or numerical artifacts.
- `release-lane` reruns strict `prod` validation, then publishes the release bundle, release-quality index, and evidence pack for review.
- Filtered CI `ctest` invocations in evidence lanes must use `--no-tests=error` to prevent false green jobs when selectors drift.
- Filtered CI `ctest` invocations in evidence lanes must select tests by normalized `TST_*` identifiers, not legacy suite-name prefixes.
- Temporary waivers and deviations must carry explicit owner, approver, and review date metadata in the canonical register.
- Release review should begin from the release-quality index before opening the full evidence pack.
- Breaking interface changes must update the canonical contract artifact and linked tests in the same review.
- Release candidates must publish an evidence pack generated from the `release-lane` commands under the selected profile.
- Runtime behavior in critical paths must be reproducible under pinned toolchain settings.
- Production C++ paths (`apps/`, `engine/`, `runtime/`, `modules/`) must not use unnamed namespaces.
- Production C++ paths must also satisfy the automated subset of the `Power of 10` profile (`goto` forbidden, `setjmp`/`longjmp` forbidden, `do-while` forbidden).
- Any requirement, tolerance, or toolchain update must update the quality artifacts in this directory in the same change.
