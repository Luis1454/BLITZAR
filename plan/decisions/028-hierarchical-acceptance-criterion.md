# Decision 028: Hierarchical Acceptance Criterion

Status: accepted
Plan version: 1.0.11

## Decision

Barnes-Hut and FMM use the same deterministic geometric acceptance rule for
an internal cell. A cell is approximated only when it does not contain the
target, its center-of-mass distance is non-zero, and

`(2 * cell.half_extent) / distance < opening_angle`.

The full cell width is used rather than the half extent. A zero opening angle
therefore disables every internal approximation and supplies the direct
reference mode. Particle ordering is Morton-key order with the original
particle index as the stable tie-breaker; refit is accepted only while every
particle remains inside its existing leaf, otherwise the tree is rebuilt.

## Consequences

- Barnes-Hut error tests can change the opening angle without changing the
  tree ownership or summation contract.
- FMM uses the same near-field boundary and differs only in the far-field
  expansion applied to accepted cells.
- Bounds, refit failures, stable ordering, and approximation error remain
  independently observable in deterministic P3 tests.
