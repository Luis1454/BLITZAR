# Decision 047: Cross-Platform Post-Merge Qualification

Status: accepted
Issue: #661
Plan version: 1.0.31

## Context

The configured CLI is an internal composition root: it consumes configuration
and metadata implementation types that are intentionally absent from the
public shared-library ABI. Building that CLI against a shared library therefore
cannot be repaired by exporting private C++ symbols. The qualification parser
also needs to build with the libc++ version supplied by the macOS runner, where
the floating-point `std::from_chars` overload is unavailable.

The final audit runs after squash merges. A source commit recorded before a
merge can remain useful for traceability while its object is no longer reachable
from the integrated `main` history.

## Decision

- `BLITZAR_BUILD_SHARED=ON` and `BLITZAR_BUILD_CLI=ON` are rejected at configure
  time. The CLI remains available in the static product build and the shared
  lane validates only the C and C++ public SDK consumers.
- Real configuration values are parsed through a classic-locale stream with
  strict full-token and finite-value validation. Integer values retain the
  allocation-free `from_chars` path.
- The final audit first resolves an integrated commit by the pull-request
  number recorded in the contract, then by a subject beginning with
  `Issue #N:`, then by an issue-style marker such as `issue-N`. If no such
  integration marker exists, it falls back to the source commit recorded in
  the contract. The audit report exposes the resolved integration commit
  separately from source-commit presence.

The boundary is covered by `TST-P0-008`, the portable real-value cases remain
covered by `TST-P2-008`, and `CHK-P0-031` covers squash-commit resolution.

## Consequences

The shared library does not gain accidental private C++ exports, macOS uses a
portable deterministic configuration path, and the strict final audit remains
valid after the repository's required squash-merge workflow. The CLI still
requires the static internal composition build by design.
