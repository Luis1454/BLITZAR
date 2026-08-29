# Decision 061: Local Neighbor Index Policy

Status: **active**
Plan version: **1.0.45**
Issue: **#674**

## Decision

The first local-neighbor structure selected for BLITZAR is a deterministic
cell-linked list with a cell width equal to the interaction radius. It is a
test-qualified policy for future local interactions, SPH, and grid paths; it
is not yet a production `src/` component.

The candidate matrix also measures a deterministic spatial hash, a 3D Hilbert
ordering baseline, and a Verlet list with a positive skin. Every candidate
must return the same directed neighbor set as the exact O(N^2) reference, with
target and source particles ordered by their original indices.

## Boundary and Ownership Contract

The qualification uses the finite non-periodic box `[-8,8]^3`. Coordinates at
the upper boundary are clamped to the final cell. Periodic replicas and Ewald
separation are excluded until the PBC contract is implemented. The original
particle index remains the stable ownership and ordering identity.

The interaction radius is `0.75`; the Verlet skin is `0.4`. A Verlet index is
rebuilt when the maximum displacement from its reference frame exceeds
`skin / 2`. Static workloads build once, while the deterministic moving
workload crosses that threshold on every frame.

## Architectural Boundary

The existing Octree remains owned by `src/trees/octree` for hierarchical and
long-range work. Its build time, cell count, memory bound, and hash are reported
as a separate baseline only. The Octree is not used as the local-neighbor
oracle, and this issue does not implement SPH, PM, TreePM, PBC, or KIFMM.

The selected cell-linked policy can later be promoted into a production module
only after a separate API, ownership, capacity, and runtime review. The test
matrix is deliberately independent of that future promotion so that local
neighbor semantics do not become coupled to long-range solver dispatch.

## Evidence

`plan/neighborhood.json` freezes four deterministic workloads and four
candidates. `tests/neighborhood` emits sixteen records. The strict evidence
runner writes generated logs and results outside the source tree and rejects
missing records, non-finite timings, incorrect neighbor sets, changed rebuild
schedules, duplicate ownership keys, and memory-bound violations.
The test target also runs an explicit boundary probe for minimum and maximum
coordinates and exact-radius neighbors before emitting the matrix.
