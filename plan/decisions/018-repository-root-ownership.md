# Decision 018: Repository Root Ownership

Status: superseded by Decision 035
Plan version: 1.0.6

## Decision

The initial P4 layout used one materialized ownership boundary for HIP runtime,
native CUDA compatibility, pinned staging, streams, and kernel launch policy.
Decision 035 replaces that boundary with the explicit
`src/accelerators/gpu/hip/{bridge,runtime,memory,launch,direct,barnes_hut}` module layout. There is
no parallel CUDA-runtime production root.

The not-yet-materialized FMM, PM, TreePM, grid, and persistence modules are
listed explicitly in `deferred_roots`. Their plan descriptions remain useful
for delivery order, but they cannot receive production code until promoted to
`roots` with tests and ownership defined.

## Consequences

- CMake GPU sources and the manifest now point to the accelerator module and
  its kernel responsibilities.
- `plan_check` verifies that repository-shape paths are covered by either a
  materialized or deferred manifest root.
- `plan_check` also rejects new top-level production directories that are not
  represented in the manifest.
- Adding a future module requires one coordinated plan change rather than an
  undocumented directory creation.
