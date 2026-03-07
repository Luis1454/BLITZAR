# Quality Assurance Baseline

This folder contains the repository-level quality baseline for high-assurance work.

## Contents

- `quality_manifest.json`: canonical quality manifest index (`metadata` + `includes`).
- `manifest/*.json`: sharded quality payloads (`evidence`, `policies`, `requirements`, `test_groups`, `crosswalk`).
- `python_tools/policies/evidence_registry.py`: resolver that reads `EVD_*` mappings from `quality_manifest.json`.
- `standards_profile.md`: NASA-first standards profile and scope.
- `fmea.md`: failure mode and effects analysis baseline.
- `tool_qualification.md`: tool confidence and qualification strategy.
- `ivv_plan.md`: independent verification and validation plan.
- `numerical_validation.md`: physics-oriented numerical acceptance criteria.
- `evidence_pack.md`: format and generation rules for release evidence bundles.

## Policy

- Primary compliance posture is NASA-first (`NPR-7150.2D`, `NASA-STD-8739.8B`) with ECSS crosswalk support.
- `prod` evidence is deterministic and strict-gated; `dev` evidence is non-qualification unless reproduced under `prod` constraints.
- Every requirement ID must be traceable to at least one verification artifact.
- Traceability and quality documents are enforced by `tests/checks/quality_check.py` against `quality_manifest.json`.
- Quality payload is loaded through deterministic include merge with cycle/duplicate-key protections.
- All quality artifacts remain constrained by repository file-size policy (target `<=200`, hard `<=300` lines).
- Evidence references remain `EVD_*` only (no hardcoded file paths in policy rows).
- Release candidates should emit an evidence pack generated from the canonical manifest and strict lane commands.
- Changes to this folder must be reviewed with the same rigor as code changes.
