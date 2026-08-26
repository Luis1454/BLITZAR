# Simulation Composition Review

Issue: #606

This review records the ownership and lifetime boundaries of `Simulation`.
It is intentionally specific to the SDK composition root. It does not create
a generic state aggregate and it does not change the public ABI.

## Ownership Matrix

| Component | Owner | Borrowed dependencies | Mutation phases | Failure authority |
| --- | --- | --- | --- | --- |
| `resources_` | `Simulation` owns `SimulationResources` by value | `MpiExchange` borrows its `MpiContext` and `DomainDecomposition` | construction, particle distribution, ghost exchange, migration, gather, HIP dispatch | `MpiContext::Status`, exchange capacity, and synchronized MPI status |
| `particle_storage_` | `Simulation` owns `ParticleStorage` by value | `ParticleBuffer`, `AccelerationBuffer`, and `KdkCheckpoint` borrow its `ParticleArena` | input commit, KDK, migration, rollback | buffer status and transaction validation |
| `source_` | `Simulation` owns the remote source buffer | none | ghost completion, remote force evaluation, abort | source capacity and dispatcher status |
| `gravity_`, `barnes_hut_` | `Simulation` owns value configuration | candidate solver borrows configuration only during construction | configuration mutators and solver rebuild | candidate validation and `Remember` |
| `solver_` | `Simulation` owns the in-place `SolverVariant` | dispatchers borrow the active solver for one step | configuration rebuild and KDK step | solver status synchronized by the active execution path |
| `integrator_` | `Simulation` owns the KDK integrator | advance requests borrow buffers and dispatchers for one call | KDK step and rollback hooks | advance status and transaction abort |
| `traversal_stacks_` | `Simulation` owns the Barnes-Hut workspace pool | advance and dispatcher requests borrow it for one call | solver preparation and force evaluation | solver capacity status |
| `particle_ids_`, `local_particle_count_` | `Simulation` owns distributed ownership metadata | packet requests borrow spans during distribution and migration | input commit, migration, rollback, gather | packet validation and synchronized migration status |
| packet buffers | `Simulation` owns each buffer by value | exchange, transaction, and gather operations borrow one phase buffer | distribution, ghost exchange, migration, rollback, gather | bounded-capacity checks before mutation |
| `last_status_`, `last_backend_` | `Simulation` owns atomic result state | no borrowed ownership | status publication after public operations | `Remember` is the single public status writer |
| `overlap_mode_`, `overlap_trace_` | `Simulation` owns test/measurement state | distributed dispatcher borrows the trace for one step | overlap test setup and distributed force evaluation | exchange and dispatcher status |

## Lifetime Order

`SimulationResources` constructs and destroys its members in this order:

```text
MpiContext -> DomainDecomposition -> MpiExchange -> Context
```

`MpiExchange` stores references to the first two objects, so they must remain
members of the same owner and must be declared before it. `ParticleStorage`
uses the equivalent order:

```text
ParticleArena -> ParticleBuffer -> AccelerationBuffer -> KdkCheckpoint
```

The three views never own the arena and cannot outlive `ParticleStorage`.
Both composition objects are non-copyable and non-movable to prevent a
reference member from being silently rebound or invalidated.

## Dependency Graph

```text
Simulation
  +-- SimulationResources
  |     +-- MpiContext
  |     +-- DomainDecomposition
  |     +-- MpiExchange
  |     +-- Context
  +-- ParticleStorage
  |     +-- ParticleArena
  |     +-- ParticleBuffer
  |     +-- AccelerationBuffer
  |     +-- KdkCheckpoint
  +-- SourceBuffer
  +-- SolverVariant and LeapfrogKdk
  +-- configuration values and work buffers
```

`SourceBuffer`, solver configuration, ownership IDs, packet buffers, and
transaction state are not grouped because they have different mutation
phases, rollback semantics, or owners. `DistributedDispatcher` remains a
non-owning per-step adapter. Its compile-time branches are strategy dispatch
for solver capabilities, not runtime state ownership.

## Regression Contract

The refactor preserves the existing lifecycle, numerical, allocation, MPI,
rollback, overlap, HIP-fallback, package, and public ABI tests. No new owner
is introduced for an existing allocation, and no public type exposes either
composition object.
