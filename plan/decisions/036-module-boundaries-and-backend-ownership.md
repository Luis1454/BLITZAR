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

- `src/core` contains only shared value, execution, status, units,
  and snapshot contracts.
- `src/particles/{arena,buffer,source}` separates allocation ownership,
  mutable SoA buffers, and remote source input. `ParticleAccess.cpp` belongs
  with the buffers it exposes; no generic access directory is retained.
- `src/trees/octree` owns the octree and the shared Morton key primitive.
  Access, construction, ordering, and properties are separate implementation
  responsibilities below that module.
- `src/solvers` owns the shared solver contract, bounded threading resources,
  and algorithm modules. Direct, Barnes-Hut, and FMM implementations own their
  algorithm-specific responsibility directories.
- `src/gpu` is the explicit GPU backend module. Runtime, memory, Direct, and
  Barnes-Hut responsibilities are separated below the HIP boundary. No HIP
  header leaks through public SDK headers, and the CPU solver remains the
  fallback/reference path.
- `src/mpi` is the MPI domain module. Its native MPI boundary remains
  isolated below `native`; runtime, collectives, domain, exchange, ghost,
  gather, and packet code use the other responsibility directories.
- `src/simulation/solver` contains solver dispatch and variant
  composition. Local, distributed, preparation, migration, overlap, and
  packet concerns live below their respective `simulation/step`
  responsibility directories. `SimParticleState` is a simulation state owner,
  not an input stage, and lives under `simulation/state`.
- `tests` mirrors externally observable responsibility boundaries, while
  `tools` remains grouped by policy role rather than implementation order.

The public C and C++ ABI paths and symbols remain unchanged. Internal include
paths, include guards, CMake source lists, manifests, ownership audit entries,
test maps, and gate fixtures must be migrated together. No compatibility copy
or forwarding file is retained at a previous path.

All C++/CUDA/header filenames are PascalCase, unique across the repository,
and identify their primary type or narrow responsibility. The responsibility
prefix is deterministic: `plan/quality.json` maps every materialized code
directory to its allowed prefix, and `tools.gates.naming_gate` rejects any
unregistered, mismatched, or colliding filename. Domain roots are aggregators;
they may contain only child modules and stable facade contracts.

## Consequences

- A domain root is an aggregator and cannot silently own an unrelated source
  file.
- Directory ownership is visible before opening a file; filenames identify the
  type or narrow responsibility and remain unique across the repository.
- HIP is named once as the backend boundary, while accelerator mechanisms and
  algorithms are expressed by their subdirectories and filenames.
- Morton ordering is a shared spatial primitive of the octree module rather
  than an unexplained sibling.
- Deferred roots such as `src/grid`, `src/io`, `src/solvers/pm`, and
  `src/solvers/treepm` remain absent until their first implementation; empty
  placeholder directories are forbidden.
- This structural change invalidates prior build and release evidence until
  the static gates, package consumer, CPU build, and CTest matrix are rerun.
