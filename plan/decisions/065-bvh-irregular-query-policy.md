# Decision 065: BVH Irregular-Query Qualification Boundary

Status: **Active**  
Issue: #677  
Plan version: **1.0.49**

## Context

The local-neighbor qualification needs a candidate for irregular spatial
queries such as collision broad phases and future short-range interactions.
The existing Octree is already the long-range gravity structure and the
cell-linked index is the selected local-neighbor policy. Replacing either
structure without a bounded workload and a deterministic reference would mix
algorithm selection with the gravitational solver contract.

## Decision

Qualify a test-only axis-aligned bounding-box index under `tests/bvh`. The
index uses an explicit stack, a longest-extent median split with particle-index
tie breaking, sorted per-target output, and a bottom-up refit that preserves
topology. Moving workloads execute both an initial-build-plus-refit pass and a
full-rebuild pass; static workloads build once. All results are compared with
the exact directed O(N^2) neighbor reference and the selected cell-linked
candidate. The existing Octree is measured separately and is never used as a
local-neighbor oracle.

The BVH remains **not selected** for production. No source under `src/`, no
Barnes-Hut/FMM force contract, no public SDK type, and no long-range gravity
dispatch is changed by this qualification. Wall-clock values describe only
index construction, refit, and local-query work; they do not establish a
physical-force or end-to-end simulation speedup.

## Acceptance Evidence

- `TST-P1-009` runs dense, sparse, clustered, and moving deterministic cases.
- `CHK-P0-044` validates the frozen contract and required non-promotion flags.
- `CHK-P0-045` rejects malformed, incomplete, and selected-candidate fixtures.
- Generated metadata, records, logs, and summaries are written outside the
  source tree.

## Promotion Requirements

Promotion would require bounded production ownership, a measured gain on the
target collision or SPH workload, a memory and rebuild policy at target scale,
and downstream numerical parity. Until those requirements are separately
qualified, the Octree remains the long-range structure and the cell-linked
index remains the selected local-neighbor policy.
