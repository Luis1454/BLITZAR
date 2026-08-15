# Physics Source Layout

BLITZAR keeps the physics tree organized by numerical responsibility. Directory
names are part of the maintenance contract and should make a file's role clear
before opening it.

## Public Contracts

```text
engine/include/physics/
  core/       particle state, vectors, force-law policy, shared views
  octree/     octree node and traversal contract
  treepm/     CPU TreePM interface
  fmm/        CPU FMM interface and metrics
  cuda/       CUDA/JIT and GPU data contracts
```

Public includes use the matching domain path, for example
`physics/treepm/TreePmCpu.hpp` or `physics/core/ParticleSystem.hpp`.

## Implementations

```text
engine/src/physics/
  core/       shared host implementations
  octree/     host octree and CPU particle-system fallback
  treepm/     CPU TreePM field and short-range correction
  fmm/        FMM build, evaluation, and quality metrics
  cuda/       host-side CUDA/JIT bridge implementation

engine/src/cuda/
  runtime CUDA files and fragments grouped by system, integration, octree,
  treepm, sph, and thermal concerns
```

CUDA fragments remain under `engine/src/cuda/fragments/` and are grouped by
execution concern: `system`, `integration`, `octree`, `treepm`, `sph`, and
`thermal`. They are included in an explicit order by `engine/src/cuda/ParticleSystem.cu`.

## Placement Rules

1. Shared data types and policies go in `core`.
2. A new numerical method gets a directory for its public contract, host
   implementation, tests, and documentation references.
3. Backend-specific code stays under the method or under `cuda` when it is a
   shared GPU runtime concern.
4. Do not add compatibility copies in the old flat `physics` directory.
5. Update the CMake source list and quality manifests in the same change as a
   move.

The layout is covered by `REQ-PHYS-001` and `REQ-PHYS-002` in the quality
traceability registry. PRs changing these paths must declare the impacted
requirement IDs in their traceability section.

## Readability Rules

Readable source is a maintenance requirement, not a cosmetic preference:

- Keep one blank line between functions and between distinct execution phases.
- Keep related declarations together, then separate setup, computation,
  synchronization, and cleanup with blank lines.
- Do not compress independent statements or responsibilities to reduce line
  count.
- Keep the aggregator short; implementation belongs in the responsibility-
  specific fragment.

The current exception is `GridBuild.inl`, which remains slightly above the
line target because its construction path is one ordered operation. It must be
split only at complete function boundaries in a follow-up change.
