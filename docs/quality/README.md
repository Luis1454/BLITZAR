# Quality Assurance Baseline

This folder contains the repository-level quality baseline for high-assurance work.

## Contents

- `quality_manifest.json`: canonical quality manifest index (`metadata` + `includes`).
- `AGENTS.md`: versioned contribution workflow artifact referenced by the NASA-first governance crosswalk.
- `manifest/*.json`: sharded quality payloads (`evidence`, `policies`, `requirements`, `test_groups`, `crosswalk`).
- `python_tools/checks/evidence_registry.py`: resolver that reads `EVD_*` mappings from `quality_manifest.json`.
- `standards_profile.md`: NASA-first standards profile and scope.
- `fmea.md`: failure mode and effects analysis baseline.
- `manifest/fmea_actions.json`: canonical owner/status/task register for FMEA mitigations.
- `tool_qualification.md`: tool confidence and qualification strategy.
- `tool_manifest.md`: generated toolchain version manifest for CI evidence lanes.
- `power_of_10.md`: repository-specific `Power of 10` coding-discipline profile with automated vs manual enforcement mapping.
- `prod_baseline.md`: explicit evidence-grade `prod` environment baseline.
- `interface_contracts.md`: versioned critical interface contracts and compatibility rules.
- `ivv_plan.md`: independent verification and validation plan.
- `numerical_validation.md`: physics-oriented numerical acceptance criteria.
- `traceability.md`: how to declare impacted requirement IDs and update the PR traceability register.
- `traceability.csv`: canonical requirement-to-surface register updated by critical functional PRs.
- `manifest/numerical_campaign.json`: fixed nightly numerical campaign definitions and thresholds.
- `evidence_pack.md`: format and generation rules for release evidence bundles.
- `release_index.md`: audit entry-point format for release-quality summaries.
- `manifest/deviations.json`: canonical waiver/deviation register for temporary exceptions.

## Policy

- Primary compliance posture is NASA-first (`NPR-7150.2D`, `NASA-STD-8739.8B`) with ECSS crosswalk support.
- `prod` evidence is deterministic and strict-gated; `dev` evidence is non-qualification unless reproduced under `prod` constraints.
- `pr-fast` is the deterministic merge gate only: workflow-driven repository checks (`check.py all`, `ruff`, `mypy`, `pytest`) plus an integration-safe `ctest` fast subset.
- `nightly-full` broadens deterministic evidence with standalone integration repeats, coverage publication, and optional GPU/numerical campaigns.
- `release-lane` packages qualification-oriented release evidence after a strict `prod` validation pass; optional GPU release lanes remain supplemental only.
- The fast `ctest` subset excludes `TST_QLT_REPO_008` and `TST_QLT_REPO_009`; those compatibility checks remain available for manual regression and full local validation.
- Every requirement ID must be traceable to at least one verification artifact.
- Traceability and quality documents are enforced by `python tests/checks/check.py quality --root .` against `quality_manifest.json`.
- Repository policy is implemented as reusable Python checks in `python_tools/checks/`, and executed through `pytest` suites under `tests/checks/suites/policy/`.
- `AGENTS.md` must remain git-tracked and mapped through `EVD_AGENTS` in the canonical evidence registry.
- The enforceable subset of the `Power of 10` profile is checked in repository policy, including open-ended loop, macro, and function-pointer constraints on production C++ paths; non-automatable rules remain review-gated.
- Strict analyzer lanes also cover ignored return values via `clang-tidy` and `[[nodiscard]]` on internal status APIs.
- Quality payload is loaded through deterministic include merge with cycle/duplicate-key protections.
- All quality artifacts remain constrained by repository file-size policy (target `<=200`, hard `<=300` lines).
- Evidence references remain `EVD_*` only (no hardcoded file paths in policy rows).
- Temporary exceptions must be recorded in `manifest/deviations.json` with owner, approver, rationale, and review date.
- Release candidates should emit a release-quality index before deep evidence review.
- Release candidates should emit an evidence pack generated from the canonical manifest and strict lane commands.
- Mission-impacting PRs must follow the IV&V checklist and non-author approval workflow.
- Changes to this folder must be reviewed with the same rigor as code changes.
