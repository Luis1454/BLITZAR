# Quality Assurance Baseline

This folder contains the repository-level quality baseline for high-assurance work.

## Contents

- `standards_profile.md`: NASA-first standards profile and scope.
- `nasa_crosswalk.csv`: mapping from external standards controls to repository evidence.
- `requirements.json`: controlled requirement catalog with unique identifiers.
- `traceability.csv`: mapping of requirements to verification artifacts.
- `test_catalog.csv`: normalized test enumeration (one code per test case).
- `fmea.md`: failure mode and effects analysis baseline.
- `tool_qualification.md`: tool confidence and qualification strategy.
- `ivv_plan.md`: independent verification and validation plan.
- `numerical_validation.md`: physics-oriented numerical acceptance criteria.

## Policy

- Primary compliance posture is NASA-first (`NPR-7150.2D`, `NASA-STD-8739.8B`) with ECSS crosswalk support.
- `prod` evidence is deterministic and strict-gated; `dev` evidence is non-qualification unless reproduced under `prod` constraints.
- Every requirement ID must be traceable to at least one verification artifact.
- Traceability and quality documents are enforced by `tests/checks/quality_check.py`.
- Changes to this folder must be reviewed with the same rigor as code changes.
