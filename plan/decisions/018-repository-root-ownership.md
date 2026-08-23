# Decision 018: Repository Root Ownership

Status: accepted
Plan version: 1.0.6

## Decision

`src/gpu` is the single materialized ownership boundary for HIP runtime,
native CUDA compatibility, pinned staging, streams, and kernel launch policy.
There is no parallel CUDA-runtime production root.

The not-yet-materialized FMM, PM, TreePM, grid, and persistence modules are
listed explicitly in `deferred_roots`. Their plan descriptions remain useful
for delivery order, but they cannot receive production code until promoted to
`roots` with tests and ownership defined.

## Consequences

- CMake GPU sources and the manifest both point to `src/gpu` and its solver
  kernel responsibility.
- `plan_check` verifies that repository-shape paths are covered by either a
  materialized or deferred manifest root.
- `plan_check` also rejects new top-level production directories that are not
  represented in the manifest.
- Adding a future module requires one coordinated plan change rather than an
  undocumented directory creation.
