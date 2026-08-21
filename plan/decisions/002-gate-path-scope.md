# Decision 002: Scope Clean-Room Path Checks

Status: accepted  
Plan version: 1.0.1

## Decision

The clean-room gate checks explicit legacy source markers rather than the
repository product name. The public include path `blitzar/` is a required part
of the new ABI and must not be treated as evidence of contamination.

## Consequences

- Public headers can be included from implementation and test targets.
- Legacy source paths remain forbidden when they are copied into code or tests.
- This is a governance correction only; the P0 feature order and architecture
  boundaries are unchanged.
