# Decision 035: Source Tree Responsibility Realignment

Status: superseded by Decision 036
Issue: #630
Plan version: 1.0.18

## Context

The clean-room implementation had already removed the legacy server boundary,
but several materialized domains still mixed leaf implementation files with
composition roots. GPU code was split between a generic backend root and a
solver-owned backend root, while SDK adapters, integration contracts, and
repository tools remained flat enough to hide ownership boundaries.

## Decision

Use responsibility-owned submodules throughout the materialized tree:

- `src/core` owns stable value and execution contracts.
- `src/particles/{arena,source,state}` owns particle storage by lifecycle role.
- `src/integration/kdk` and `src/physics/gravity` own their single algorithms.
- `src/trees/{ordering,octree}` keeps Morton ordering separate because domain
  partitioning and octree construction both consume it.
- `src/gpu/{bridge,runtime,memory,launch,direct,barnes_hut}` owns all optional GPU
  implementation details; CPU solver code remains under `src/solvers`.
- `src/mpi` is divided into context, collectives, domain, exchange,
  gather, and native adapter responsibilities.
- `src/simulation` owns composition, configuration, facade, input, step, and
  transaction behavior; `src/sdk/{c,cpp}` contains only public SDK adapters.
- Tests are grouped by contract or feature, and tools are grouped by their
  policy role. Tool commands use Python modules so imports do not depend on a
  flat directory.

No compatibility copies or forwarding files are retained at the old paths.
The public ABI headers and their symbols remain unchanged. The manifest,
CMake source lists, ownership audit, include paths, and qualification commands
must describe the same tree.

## Consequences

- A domain root is an aggregator, not an unowned implementation bucket.
- Directory names identify ownership; file names identify the primary type or
  narrowly scoped adapter and remain unique across the repository.
- Moving a file requires updating its include contract, guard, build entry,
  test map, ownership registry, and architecture review in the same change.
- Qualification must be rerun after the realignment because path ownership and
  tool entrypoints are part of the frozen repository contract.
