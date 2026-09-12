# Decision 075: Explicit missing source/test directory policy

Status: accepted
Issue: QUALITY-003
Plan version: 1.0.59

## Context

Decision 074 introduced `.keep` files to make reserved empty test
directories versionable. That creates a persistent repository artifact which
can survive after the real test directory is populated. The artifact is not
part of the source or test responsibility itself.

## Decision

Remove the `.keep` convention. The recursive source/test symmetry gate remains
strict by default, but it accepts a missing counterpart only when the exact
source path, exact test path, and a non-empty reason are declared in
`directory_symmetry.allowed_missing` in `plan/quality.json`.

An allowed missing entry is consumed only while one side is present and the
other side is absent. It becomes a gate failure when the exception is omitted,
when the opposite side is added, or when the exception is stale. Extra test
directories remain errors and cannot be hidden by the missing-directory
allowlist.

The current KIFMM transition is declared explicitly:

```json
{
  "source": "src/solvers/fmm/kifmm",
  "tests": "tests/fmm/kifmm",
  "reason": "KIFMM tests will be introduced with the variant layout migration"
}
```

No placeholder directory or placeholder file is created. When the KIFMM test
directory is materialized, this exception must be removed in the same change.

## Acceptance

- `CHK-P0-050` rejects every undeclared missing or extra counterpart.
- `CHK-P0-050` accepts only the exact declared KIFMM missing counterpart.
- `CHK-P0-050` rejects missing reasons, path mismatches, duplicate entries,
  and stale allowances.
- `CHK-P0-051` covers the allowed, rejected, extra, stale, and malformed cases.
- `plan-check` requires the symmetry policy and both registered checks, so the
  rule cannot be removed silently from the frozen plan.
