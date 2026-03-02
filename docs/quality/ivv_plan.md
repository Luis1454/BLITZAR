# IV&V Plan Baseline

Independent Verification and Validation (IV&V) in this repository is process-focused and evidence-driven.

## Standards Alignment

- Primary: `NPR-7150.2D` and `NASA-STD-8739.8B`.
- Crosswalk support: `ECSS-E-ST-40C`, `ECSS-Q-ST-80C`, `ECSS-E-ST-40-07C`.

## Independence Model

- Development and review cannot be performed by the same person for mission-impacting changes.
- Protocol, runtime recovery, and numerical threshold updates require independent reviewer approval.
- CI evidence is necessary but not sufficient; reviewer sign-off is mandatory.

## Verification Stages

1. Requirement update:
   - update `quality_manifest.json` requirement and traceability sections.
2. Implementation review:
   - verify requirement IDs and impacted artifacts are explicit.
3. Automated verification:
   - run strict PR gate (`check.py`, strict build, analyzer, deterministic fast-subset tests, quality baseline checks).
4. Extended nightly verification:
   - execute broader deterministic scopes and publish coverage and logs.
5. Closure:
   - confirm evidence links remain valid for each affected requirement.

## Mandatory Evidence per Change

- requirement IDs touched
- tests and checks executed
- analyzer result status
- reviewer identity independent from author
- profile used (`prod` qualification evidence or `dev` exploratory evidence)

## Escalation Triggers

- repeated nightly instability on the same requirement area
- unresolved analyzer findings in strict lane
- numerical threshold changes without validation evidence
