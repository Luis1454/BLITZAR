# Static Audit Remediation Plan

This plan treats the March 2026 static audit reports as serious input and converts them into a repository-tracked remediation program.

Inputs:
- `BLITZAR_rapport_analyse.md`
- `BLITZAR_rapport_addendum.md`
- local repository state on `main`

Working rule:
- No finding is dismissed by default.
- Every finding must be classified as `confirmed`, `needs verification`, or `already remediated`.
- Any finding kept open must map to a GitHub issue, a quality artifact, or an explicit verification task.

## Classification Rules

`confirmed`
- The repository state directly supports the report claim.

`needs verification`
- The claim is plausible or historically documented, but the repository state alone does not prove it is still reproducible.

`already remediated`
- The claim was valid earlier but is no longer true on current `main`.

## Immediate Governance Correction

The audits correctly identify a governance failure: [MODIFS.md](C:/Users/luisf/EpitechPromo2027/CUDA-GRAVITY-SIMULATION-restored/MODIFS.md) contains critical backlog items that are not guaranteed to be mirrored in GitHub issues or in the quality baseline.

Immediate rule from this point forward:
- GitHub issues are the authoritative task tracker.
- `docs/quality/` is the authoritative quality baseline.
- `MODIFS.md` is a scratchpad only and must not remain the only location for any `P0` or `P1` item.

## Finding Matrix

| Finding | Status | Evidence | Required action |
|---|---|---|---|
| Qt integration crash `0xc0000409` mentioned in `MODIFS.md` | already remediated | [MODIFS.md](C:/Users/luisf/EpitechPromo2027/CUDA-GRAVITY-SIMULATION-restored/MODIFS.md), focused issue [#301](https://github.com/Luis1454/BLITZAR/issues/301), validation command `cmake -S tests -B build-issue301 -G Ninja -DCMAKE_BUILD_TYPE=Debug -DGRAVITY_PROFILE=dev -DCMAKE_PREFIX_PATH=C:/Qt/6.8.2/msvc2022_64`, build command `cmake -E env "PATH=C:/Qt/6.8.2/msvc2022_64/bin;%PATH%" cmake --build build-issue301 --target gravityQtMainWindowGTests --parallel 8`, test command `cmake -E env "PATH=C:/Qt/6.8.2/msvc2022_64/bin;%PATH%" build-issue301\\gravityQtMainWindowGTests.exe --gtest_color=yes` | Current `main` no longer reproduces the crash; keep `gravityQtMainWindowGTests` green as the regression proof and do not reopen unless a fresh deterministic repro exists. |
| SPH mode can explode numerically | confirmed | [MODIFS.md](C:/Users/luisf/EpitechPromo2027/CUDA-GRAVITY-SIMULATION-restored/MODIFS.md), focused issue [#302](https://github.com/Luis1454/BLITZAR/issues/302), open hardening tracker [#72](https://github.com/Luis1454/BLITZAR/issues/72) | Add deterministic SPH acceptance scenarios and block any overclaim of production readiness until evidence exists. |
| GPU octree exactness/performance remains unfinished | confirmed | [MODIFS.md](C:/Users/luisf/EpitechPromo2027/CUDA-GRAVITY-SIMULATION-restored/MODIFS.md), focused issue [#303](https://github.com/Luis1454/BLITZAR/issues/303), open hardening tracker [#72](https://github.com/Luis1454/BLITZAR/issues/72) | Keep as `P0`; add parity harness and tolerances against pairwise reference. |
| VTK snapshot format lacks formal documentation | confirmed | [MODIFS.md](C:/Users/luisf/EpitechPromo2027/CUDA-GRAVITY-SIMULATION-restored/MODIFS.md), focused issue [#304](https://github.com/Luis1454/BLITZAR/issues/304) | Publish a versioned snapshot format note. |
| Deterministic server regression tests are missing as a closed product gate | confirmed | [MODIFS.md](C:/Users/luisf/EpitechPromo2027/CUDA-GRAVITY-SIMULATION-restored/MODIFS.md), focused issue [#305](https://github.com/Luis1454/BLITZAR/issues/305), open test gap [#84](https://github.com/Luis1454/BLITZAR/issues/84) | Add fixed-seed replay evidence and clear pass/fail contracts. |
| `MODIFS.md` is a parallel shadow backlog | confirmed | [MODIFS.md](C:/Users/luisf/EpitechPromo2027/CUDA-GRAVITY-SIMULATION-restored/MODIFS.md) | Mirror critical items into GitHub issues and de-authorize `MODIFS.md` as a canonical backlog. |
| `rust-toolchain.toml` is Windows-only | confirmed | [rust-toolchain.toml](C:/Users/luisf/EpitechPromo2027/CUDA-GRAVITY-SIMULATION-restored/rust-toolchain.toml), focused issue [#306](https://github.com/Luis1454/BLITZAR/issues/306) | Add Linux target coverage and explain the selected toolchain version. |
| `rust-toolchain.toml` version may be invalid for a clean machine | needs verification | [rust-toolchain.toml](C:/Users/luisf/EpitechPromo2027/CUDA-GRAVITY-SIMULATION-restored/rust-toolchain.toml), focused issue [#306](https://github.com/Luis1454/BLITZAR/issues/306) | Validate the pinned version against actual bootstrap on clean Windows and Linux runners. |
| `CUDA_ARCH=native` is not suitable for redistributable bundles | confirmed | [Makefile](C:/Users/luisf/EpitechPromo2027/CUDA-GRAVITY-SIMULATION-restored/Makefile) | Introduce an explicit distribution architecture matrix for packaging/release builds. |
| `quality-local` does not cover the same Python subset as CI | confirmed | [Makefile](C:/Users/luisf/EpitechPromo2027/CUDA-GRAVITY-SIMULATION-restored/Makefile), [docs/quality/README.md](C:/Users/luisf/EpitechPromo2027/CUDA-GRAVITY-SIMULATION-restored/docs/quality/README.md) | Either align local wrapper behavior or document the split more explicitly. |
| `.gitignore` misses local runtime artifacts | confirmed | [.gitignore](C:/Users/luisf/EpitechPromo2027/CUDA-GRAVITY-SIMULATION-restored/.gitignore), tracked [build_output.txt](C:/Users/luisf/EpitechPromo2027/CUDA-GRAVITY-SIMULATION-restored/build_output.txt), focused issue [#307](https://github.com/Luis1454/BLITZAR/issues/307) | Add ignores for local runtime artifacts and remove tracked leftovers. |
| GitHub Actions are not SHA-pinned | already remediated | [release-lane.yml](C:/Users/luisf/EpitechPromo2027/CUDA-GRAVITY-SIMULATION-restored/.github/workflows/release-lane.yml), [security-codeql.yml](C:/Users/luisf/EpitechPromo2027/CUDA-GRAVITY-SIMULATION-restored/.github/workflows/security-codeql.yml) | Keep enforced by policy; no new work needed beyond regression protection. |
| Python CI tooling is unpinned | already remediated | [.github/ci/requirements-py312.txt](C:/Users/luisf/EpitechPromo2027/CUDA-GRAVITY-SIMULATION-restored/.github/ci/requirements-py312.txt) | Keep enforced; no new work needed beyond parity documentation. |
| CodeQL/security scanning is absent | already remediated | [security-codeql.yml](C:/Users/luisf/EpitechPromo2027/CUDA-GRAVITY-SIMULATION-restored/.github/workflows/security-codeql.yml) | Keep active. |
| Portable Windows bundle packaging is missing | already remediated | [release-lane.yml](C:/Users/luisf/EpitechPromo2027/CUDA-GRAVITY-SIMULATION-restored/.github/workflows/release-lane.yml), [release_bundle.py](C:/Users/luisf/EpitechPromo2027/CUDA-GRAVITY-SIMULATION-restored/python_tools/ci/release_bundle.py) | Keep release smoke validation active. |
| `make clean` is missing | already remediated | [runtime.mk](C:/Users/luisf/EpitechPromo2027/CUDA-GRAVITY-SIMULATION-restored/make/runtime.mk) | None. |
| `gravity.comp` is treated as an active production path | needs verification | unreferenced in current build/docs tree | Verify whether the file is dead prototype code; if dead, quarantine or remove it explicitly. |
| GPU runner readiness gives a false sense of coverage | needs verification | [gpu-runner-health.yml](C:/Users/luisf/EpitechPromo2027/CUDA-GRAVITY-SIMULATION-restored/.github/workflows/gpu-runner-health.yml), [nightly-full.yml](C:/Users/luisf/EpitechPromo2027/CUDA-GRAVITY-SIMULATION-restored/.github/workflows/nightly-full.yml), [performance-benchmark.yml](C:/Users/luisf/EpitechPromo2027/CUDA-GRAVITY-SIMULATION-restored/.github/workflows/performance-benchmark.yml) | Audit actual runner inventory and last artifact contents before drawing conclusions. |

## Execution Order

### Wave 1: Stop governance drift

1. Mirror each critical `P0` from `MODIFS.md` into GitHub issues.
2. Mark `MODIFS.md` as a scratchpad, not a source of truth.
3. Add this remediation plan to the quality baseline index.

Exit evidence:
- every `P0` in `MODIFS.md` has either a linked issue or a documented closure note
- no critical item exists only in `MODIFS.md`

### Wave 2: Close the product-core gaps

1. Reproduce or clear the Qt crash claim.
2. Fix or bound SPH instability with deterministic scenarios.
3. Add explicit octree parity validation against pairwise reference.
4. Add deterministic server regression suites at fixed seed and fixed step count.
5. Publish formal VTK snapshot format documentation.

Exit evidence:
- deterministic tests and numerical artifacts for the chosen demo scenarios
- no unresolved `P0` remains in product-core without an owning issue and a repro/evidence note

### Wave 3: Harden toolchain and reproducibility

1. Fix `rust-toolchain.toml` target strategy and validate bootstrap on Linux and Windows.
2. Replace `CUDA_ARCH=native` for release/distribution-oriented build paths.
3. Align local quality wrappers with CI expectations or document the split precisely.
4. Clean `.gitignore` and remove tracked local artifacts.

Exit evidence:
- clean bootstrap note for Rust on both CI families
- explicit release/distribution CUDA architecture policy
- no tracked local build artifact at repo root

### Wave 4: Verify the GPU truthfulness story

1. Inspect recent GPU runner health artifacts.
2. Inspect nightly and performance benchmark artifacts, not just workflow durations.
3. If runners are unavailable, make the fallback status explicit and impossible to misread as successful GPU evidence.

Exit evidence:
- documented answer to "do we currently have real GPU evidence?"
- workflow summaries that distinguish `executed`, `skipped`, and `fallback`

### Wave 5: Burn down structural debt visible to reviewers

1. Continue open deviation closures [#288](https://github.com/Luis1454/BLITZAR/issues/288) and [#289](https://github.com/Luis1454/BLITZAR/issues/289).
2. Keep narrowing oversized runtime and UI files only where the responsibility split is real.
3. Do not claim high-assurance closure until the product-core and CI truthfulness items above are addressed.

## Non-Negotiable Reporting Rules

- No report finding may be marked `already remediated` without a direct repository reference.
- No report finding may stay `needs verification` for more than one planning cycle without an explicit owner.
- Any public quality claim must be downgraded immediately if the supporting evidence is optional, stale, or skipped in practice.

## Immediate Follow-Up Tasks

- Execute focused issues [#302](https://github.com/Luis1454/BLITZAR/issues/302), [#303](https://github.com/Luis1454/BLITZAR/issues/303), [#304](https://github.com/Luis1454/BLITZAR/issues/304), and [#305](https://github.com/Luis1454/BLITZAR/issues/305) for the remaining former `MODIFS.md`-only `P0` items.
- Execute focused issue [#306](https://github.com/Luis1454/BLITZAR/issues/306) for `rust-toolchain.toml` portability/validity.
- Execute focused issue [#307](https://github.com/Luis1454/BLITZAR/issues/307) for `.gitignore` cleanup and tracked local artifact removal.
- Review the last GPU runner artifacts before making any further public claim about GPU evidence depth.
