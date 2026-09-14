# Decision 086: production cell-linked neighbor index (P8-SPH-002)

Status: accepted
Plan version: 1.0.70

## Context

Issue 720 (`P8-SPH-002`) implements the production local neighbor index for
SPH and future local physics, using the approved cell-linked candidate of the
neighborhood contract. It must match a brute-force reference within the
declared inclusive-squared-distance rule, be deterministic and non-allocating
after construction, and detect all capacity overflow before any result
mutation.

## Decision

Promote the cell-linked candidate into a production `NeighborIndex` class in
`src/physics/neighbors/` (decision id `P8-SPH-002`). The index operates on
particle positions only, stores cell occupancy, entries, neighbors, and
neighbor offsets in preallocated vectors sized by the declared capacity and
geometry at construction; after warmup it never allocates. `Build` runs a
strict three-phase protocol: geometry validation, O(N²) exact-neighbor-count
preflight (read-only) detecting both particle and neighbor capacity overflow
before any internal buffer mutation, then entry placement and ascending
per-target sort. The resulting neighbor lists are bit-deterministic and match
the brute-force reference, with empty support, exact-radius, finite-box, and
boundary-clamp behavior defined.

A qualified TST-P10-001 oracle test (`NeighborIndexTest`) validates
brute-force equality, determinism, ascending ordering, particle and neighbor
capacity overflow rejection before mutation, finite-box and exact-radius
inclusion, and empty / span-mismatch / non-finite rejection.

## Constraints

- The geometry-derived cell count is bounded by `capacity^2` to keep the
  index genuinely bounded; the production SPH regime satisfies this.
- GPU and MPI views reuse the same logical neighbor index through the shared
  `NeighborIndex` boundary; physical parity is qualified in their own phases
  (P4, P8).
- Gravity-only paths and all pre-existing test ids are unchanged.
- No dynamic allocation occurs after construction.