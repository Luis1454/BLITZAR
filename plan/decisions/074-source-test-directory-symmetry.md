# Decision 074: Source and test responsibility-directory symmetry

Status: accepted
Issue: QUALITY-003
Plan version: 1.0.58

## Context

The repository already models solver families and variants as nested
responsibility roots. The CPU FMM family is materialized at
`src/solvers/fmm`, with KIFMM below it at `src/solvers/fmm/kifmm`, while the
qualification files were still flattened into `tests/fmm`. That flattening
allows source and test trees to drift as more variants are added.

## Decision

Freeze recursive source/test directory symmetry through the
`directory_symmetry` policy in `plan/quality.json`. Every declared
`src/...`/`tests/...` pair must contain the same relative directory set in both
directions. The rule is intentionally directory-scoped: the directory carries
the family and variant responsibility, while each file retains a unique local
responsibility name.

An empty reserved directory is represented by a zero-content `.keep` marker.
The marker is required for an empty mirrored directory, forbidden in a
non-empty directory, and ignored by architecture/source discovery. This keeps
future variant directories versionable without inventing placeholder source or
test code.

The first frozen pair is the FMM family:

```text
src/solvers/fmm/       <-> tests/fmm/
src/solvers/fmm/kifmm/ <-> tests/fmm/kifmm/
```

The GPU FMM capability remains deferred and receives no directory until its
implementation contract is accepted.

## Acceptance

- `CHK-P0-050` rejects missing and extra mirrored directories.
- `CHK-P0-050` requires `.keep` for empty reserved directories and rejects
  non-empty or non-empty-content markers.
- `CHK-P0-051` covers the valid mirror and every negative marker/symmetry case.
- The current KIFMM test directory is present and versionable through
  `tests/fmm/kifmm/.keep`.
- The naming policy keeps C++ filenames globally unique and applies the KIFMM
  responsibility prefix to files placed below `tests/fmm/kifmm`.
