# Decision 003: Materialized Root Scope

Status: accepted  
Plan version: 1.0.2

## Decision

The manifest separates non-empty roots that are materialized in the current
clean-room tree from future roots that are explicitly deferred. The quality
gate rejects missing or empty materialized roots and rejects deferred roots
that become materialized without being promoted into `roots`.

## Consequences

- The repository does not create placeholder directories for future phases.
- Adding a new production domain requires moving its path from
  `deferred_roots` to `roots` in the same plan change.
- The plan remains truthful during the staged rewrite.
