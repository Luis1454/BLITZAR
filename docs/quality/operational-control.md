# Operational Control Framework

This document defines how BLITZAR is steered while product-core, quality, and structural work remain active.

## Control Rules

- GitHub issues are the only authoritative task tracker.
- `docs/quality/` is the only authoritative quality baseline.
- Local scratchpads are non-authoritative; no `P0` or `P1` item may live there alone.
- One branch targets one issue, from `main` only.
- No public claim may exceed the current evidence.
- Coverage is a steering signal and must be reviewed every planning cycle, but it is never accepted as a substitute for deterministic scenario validation.

## Execution States

Every critical issue must be in exactly one of these states:

- `unverified`: claim exists, no current repro on `main`
- `reproduced`: current repro exists with exact command/build context
- `bounded`: unsupported or risky behavior has been explicitly constrained and documented
- `fixed`: behavior is corrected and protected by tests or evidence
- `closed`: merged to `main`, issue closed, baseline updated

Transitions are strict:

- `unverified -> reproduced` requires an exact repro command or artifact
- `unverified -> fixed` is forbidden
- `reproduced -> fixed` requires a regression proof
- `fixed -> closed` requires merge to `main` and synchronized docs/quality state

## Daily Steering Loop

The active control board has only three columns:

- `Now`: one active issue
- `Next`: one queued issue
- `Blocked`: only items waiting on a confirmed external dependency

Rules:

- do not open parallel product-core implementation branches
- do not start structural refactors while `Now` is a product-core blocker
- if a task reveals new failures, open focused issues and return the parent task to `reproduced` or `bounded`

## Evidence Rules

Each critical issue must carry:

- exact repro/build/test commands
- deterministic inputs where applicable
- owner issue reference
- proof location in code, test, workflow, or generated artifact

Minimum evidence by class:

- runtime crash: failing or passing command + log + current conclusion
- physics drift/parity: fixed-seed scenario + stored threshold + automated verdict
- UI/runtime refresh: automated test and screenshot only when automation is insufficient
- CI/release: workflow run plus explicit `executed`, `skipped`, or `fallback` status

## Coverage As A Central Steering Signal

Coverage is central because it exposes unexecuted risk, not because it proves correctness.

Operational rules:

- review line/function/branch coverage every planning cycle from the nightly dashboard
- coverage regressions on critical subsystems must be investigated even when tests stay green
- low coverage on non-critical paths is acceptable only when the path is explicitly out of scope
- no percentage target alone may close an issue

Priority review surfaces:

- `engine/src/server`
- `runtime/src/client`
- `runtime/src/protocol`
- `modules/qt/ui`
- solver-specific physics paths

Coverage is considered healthy only when paired with deterministic regression suites for the same subsystem.

## Escalation Rules

- if a `P0` stays `unverified` for more than one planning cycle, it must get an owner and exact next verification step
- if a claim is contradicted by current evidence, the public doc must be downgraded in the same change
- if CI falls back or skips an evidence lane, the summary must say so explicitly
- if a task creates broad collateral failures, stop and re-scope instead of widening the branch
