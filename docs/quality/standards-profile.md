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
  - client modules, if explicitly built under `prod`, must be startup-only, allowlisted, manifest-verified, and checksum-verified before loading;
  - strict PR quality gate must be green before merge.
  - CI lanes must force `-DGRAVITY_PROFILE=prod` for qualification evidence.
  - repository policy checks reject evidence workflow configure commands that omit `-DGRAVITY_PROFILE=prod`.
  - evidence-grade environment assumptions are fixed by `docs/quality/production-baseline.md`.
- `dev` profile (iteration path):
  - broader experimentation allowed;
  - Rust workspace crates may mirror protocol or runtime contracts here before they are wired into the qualified CI lanes;
  - local strict preflight must still execute pinned Cargo formatting and unit tests when Rust workspace crates are present;
  - results are not qualification evidence unless reproduced in `prod` profile constraints.

Build switch:

- `-DGRAVITY_PROFILE=prod` for qualification-oriented builds.
- `-DGRAVITY_PROFILE=dev` for iteration builds.

## Mandatory Evidence Set

- Contribution workflow artifact: `AGENTS.md`.
- Canonical quality manifest: `docs/quality/quality_manifest.json`.
- IV&V plan: `docs/quality/ivv-plan.md`.
- Failure analysis: `docs/quality/fmea.md`.
- Numerical acceptance policy: `docs/quality/numerical-validation.md`.
- Tool confidence strategy: `docs/quality/tool-qualification.md`.
- Power of 10 coding profile: `docs/quality/power-of-10.md`.
- Qualified `prod` baseline: `docs/quality/production-baseline.md`.
- Critical interface contracts: `docs/quality/interface-contracts.md`.
- Deviation register: `docs/quality/manifest/deviations.json`.
- Release quality index format: `docs/quality/release-index.md`.
- Strict merge gate definition: `.github/workflows/pr-fast.yml` and `.github/workflows/pr-fast-quality-gate.yml`.
- Extended deterministic evidence lanes: `.github/workflows/nightly-full.yml` and `.github/workflows/release-lane.yml`.
- Release evidence pack format: `docs/quality/evidence-pack.md`.

## Determinism Baseline

- Fixed-input regression tests are the authoritative evidence.
- `pr-fast` executes repository policy gates, analyzer checks, and a deterministic fast subset for merge safety.
- `main` must only receive commits that are traceable to merged `issue/<N>-<slug>` pull requests, either directly or through a merge commit whose introduced compare-range commits remain fully traceable to merged issue branches.
- Human-authored pull requests must follow `issue/<N>-<slug>` branch policy; automated Dependabot update PRs may use their native `dependabot/<ecosystem>/<dependency>` branch form when the bot-authored title/body remain intact.
- `nightly-full` extends deterministic evidence with repeated standalone integration runs, coverage publication, FMEA status snapshots, and optional GPU full-suite or numerical artifacts.
- `release-lane` reruns strict `prod` validation, then publishes the release bundle, release-quality index, and evidence pack for review.
- Security CI coverage must include CodeQL code scanning, pull-request dependency vulnerability review, and an automated SBOM for packaged release artifacts.
- Hosted evidence lanes must pin runner images and Python versions explicitly, and job names must expose the platform/toolchain axis for failure triage.
- Hosted evidence lanes must also pin external GitHub Actions by full commit SHA and install CI Python tooling from a repo-owned pinned manifest.
- Optional self-hosted GPU lanes must emit a readiness report plus an explicit hosted fallback record when runner capacity is unavailable.
- Filtered CI `ctest` invocations in evidence lanes must use `--no-tests=error` to prevent false green jobs when selectors drift.
- Filtered CI `ctest` invocations in evidence lanes must select tests by normalized `TST_*` identifiers, not legacy suite-name prefixes.
- Temporary waivers and deviations must carry explicit owner, approver, and review date metadata in the canonical register.
- Single-maintainer critical-path PRs may use an explicit solo IV&V waiver token plus `DEV-SOLO-IVV` or `WVR-SOLO-IVV`; otherwise a non-author approval remains mandatory.
- Release review should begin from the release-quality index before opening the full evidence pack.
- Breaking interface changes must update the canonical contract artifact and linked tests in the same review.
- Breaking IPC changes must update both `docs/server-protocol.md` and `runtime/protocol/server-protocol-schema.json` in the same review.
- Release candidates must publish an evidence pack generated from the `release-lane` commands under the selected profile.
- Runtime behavior in critical paths must be reproducible under pinned toolchain settings.
- Dynamic client-module verification in `prod` must remain deterministic: allowlist, `apiVersion`, product metadata, and binary `sha256` are all part of the loader contract.
- Mixed-language seams must remain narrow and deterministic: the compute-core FFI may expose only opaque handles plus POD request/status/snapshot structs, never C++/CUDA implementation types.
- Rust workspace additions must pin `rustc` through `rust-toolchain.toml`, commit `Cargo.lock`, and keep the mixed-language seam deterministic; local strict preflight must execute Cargo checks, while Cargo-built artifacts remain `dev` evidence until a later issue wires them into the qualified `prod` lanes and tool manifest.
- Optional web transport adapters must preserve `server-json-v1` command semantics exactly and remain outside the compute-core qualification boundary unless a future issue explicitly qualifies them under `prod`.
- Interactive performance presets may tune snapshot cadence, client draw cap, energy sampling, and bounded substep policy, but must not silently change solver or integrator mode.
- Production C++ paths (`apps/`, `engine/`, `runtime/`, `modules/`) must not use unnamed namespaces.
- Production C++ paths must also satisfy the automated subset of the `Power of 10` profile (`goto`, `setjmp`/`longjmp`, `do-while`, open-ended `while(true)`, non-structural object-like macros, and non-ABI function-pointer typedefs are forbidden).
- Repository policy keeps file-size thresholds as a decomposition signal (`<=200` target, strong alert `>300`) and supplements them with warnings for oversized functions, excessive function counts, and lightweight complexity signals rather than rewarding artificial wrapper splits.
- Physics stability constants (max acceleration, softening floor, etc.) are exposed through `SimulationConfig` to allow deterministic tuning of solver boundaries without recompilation.
- Configuration parsing and runtime diagnostics must expose explicit SI units for physical quantities unless a field name explicitly carries another unit such as `_ms` or `fps`.
- Repository C++ headers must use strict include guards, not `#pragma once`.
- Repository C++ sources must not use preprocessor conditionals outside header include guards; platform seams must be selected by the build graph, not `#if/#else` branches.
- Repository C++ sources must not define macros outside header include guards.
- Strict analyzer lanes also enforce ignored-return-value coverage for internal status APIs through `clang-tidy` and targeted `[[nodiscard]]` annotations.
- Any requirement, tolerance, or toolchain update must update the quality artifacts in this directory in the same change.



