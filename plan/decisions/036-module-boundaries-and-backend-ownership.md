# Decision 036: Module Boundaries and Backend Ownership

Status: accepted
Issue: #630
Plan version: 1.0.20

## Context

The first source-tree pass grouped the main domains, but several modules still
mixed contracts, execution resources, and algorithm-specific implementation
files at the same directory level. The GPU backend also needed an explicit
backend boundary so a future accelerator cannot be confused with the HIP
implementation. These are ownership problems, not line-count problems.

## Decision

Use the following responsibility grammar for the materialized tree:

- `src/core/contracts` contains only shared value, execution, status, units,
  and snapshot contracts.
- `src/particles/{arena,buffers,source}` separates allocation ownership,
  mutable SoA buffers, and remote source input. `ParticleAccess.cpp` belongs
  with the buffers it exposes; no generic access directory is retained.
- `src/trees/ordering` owns the shared Morton key primitive because both the
  octree and MPI domain partitioning consume it. `src/trees/octree` owns the
  octree type, with `access`, `construction`, `ordering`, and `properties`
  responsibilities below it.
- `src/solvers/{contracts,threading}` owns shared solver contracts and bounded
  traversal resources. Direct, Barnes-Hut, and FMM implementations own their
  algorithm-specific responsibility subdirectories.
- `src/accelerators/gpu/hip` is the explicit GPU backend module. Its bridge,
  runtime, memory, launch, Direct, and Barnes-Hut responsibilities are
  separated below the HIP boundary. No HIP header leaks through public SDK
  headers, and the CPU solver remains the fallback/reference path.
- `src/parallel/mpi` is the MPI domain module. Its native MPI boundary remains
  isolated below `native`; collectives, context, domain, exchange, and gather
  code use the other responsibility directories.
- `src/simulation/composition/solver` contains solver dispatch and variant
  composition. Local, distributed, preparation, migration, overlap, and
  packet concerns live below their respective `simulation/step`
  responsibility directories. `ParticleStorage` is a simulation state owner,
  not an input stage, and lives under `simulation/storage`.
- `tests` mirrors externally observable responsibility boundaries, while
  `tools` remains grouped by policy role rather than implementation order.

The public C and C++ ABI paths and symbols remain unchanged. Internal include
paths, include guards, CMake source lists, manifests, ownership audit entries,
test maps, and gate fixtures must be migrated together. No compatibility copy
or forwarding file is retained at a previous path.

## Consequences

- A domain root is an aggregator and cannot silently own an unrelated source
  file.
- Directory ownership is visible before opening a file; filenames identify the
  type or narrow responsibility and remain unique across the repository.
- HIP is named once as the backend boundary, while accelerator mechanisms and
  algorithms are expressed by their subdirectories and filenames.
- Morton ordering is documented as a shared spatial primitive rather than an
  unexplained sibling of the octree.
- This structural change invalidates prior build and release evidence until
  the static gates, package consumer, CPU build, and CTest matrix are rerun.
