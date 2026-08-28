# Simulation Composition Review

Issue: #606

This review records the ownership and lifetime boundaries of `Sim`.
It is intentionally specific to the SDK composition root. It does not create
a generic state aggregate and it does not change the public ABI.

## Ownership Matrix

| Component | Owner | Borrowed dependencies | Mutation phases | Failure authority |
| --- | --- | --- | --- | --- |
| `runtime_` | `Sim` owns `SimRuntime` by value | `MpiExchange` borrows its `MpiContext` and `MpiDomainDecomposition` | construction, particle distribution, ghost exchange, migration, gather, HIP dispatch | `MpiContext::Status`, exchange capacity, and synchronized MPI status |
| `particle_state_` | `Sim` owns `SimParticleState` by value | `ParticleBuffer`, `ParticleAccelerationBuffer`, and `KdkCheckpoint` borrow its `ParticleArena` | input commit, KDK, migration, rollback | buffer status and transaction validation |
| `particle_source_` | `Sim` owns the remote source buffer | none | ghost completion, remote force evaluation, abort | source capacity and force-provider status |
| `gravity_`, `barnes_hut_` | `Sim` owns value configuration | candidate solver borrows configuration only during construction | configuration mutators and solver rebuild | candidate validation and `Remember` |
| `solver_` | `Sim` owns the in-place `SolverVariant` | force providers borrow the active solver for one step | configuration rebuild and KDK step | solver status synchronized by the active execution path |
| `integrator_` | `Sim` owns the KDK integrator | advance requests borrow buffers and force providers for one call | KDK step and rollback hooks | advance status and transaction abort |
| `particle_ids_`, `local_particle_count_` | `Sim` owns distributed ownership metadata | packet requests borrow spans during distribution and migration | input commit, migration, rollback, gather | packet validation and synchronized migration status |
| packet buffers | `Sim` owns each buffer by value | exchange, transaction, and gather operations borrow one phase buffer | distribution, ghost exchange, migration, rollback, gather | bounded-capacity checks before mutation |
| `last_status_`, `last_backend_` | `Sim` owns atomic result state | no borrowed ownership | status publication after public operations | `Remember` is the single public status writer |
| `overlap_mode_`, `overlap_trace_` | `Sim` owns test/measurement state | distributed force provider borrows the trace for one step | overlap test setup and distributed force evaluation | exchange and provider status |

## Lifetime Order

`SimRuntime` constructs and destroys its members in this order:

```text
MpiContext -> MpiDomainDecomposition -> MpiExchange -> GpuContext
```

`MpiExchange` stores references to the first two objects, so they must remain
members of the same owner and must be declared before it. `SimParticleState`
uses the equivalent order:

```text
ParticleArena -> ParticleBuffer -> ParticleAccelerationBuffer -> KdkCheckpoint
```

The three views never own the arena and cannot outlive `SimParticleState`.
Both composition objects are non-copyable and non-movable to prevent a
reference member from being silently rebound or invalidated.

## Dependency Graph

```text
Sim
  +-- SimRuntime
  |     +-- MpiContext
  |     +-- MpiDomainDecomposition
  |     +-- MpiExchange
  |     +-- GpuContext
  +-- SimParticleState
  |     +-- ParticleArena
  |     +-- ParticleBuffer
  |     +-- ParticleAccelerationBuffer
  |     +-- KdkCheckpoint
  +-- ParticleSourceBuffer
  +-- SolverVariant, force providers, and KdkLeapfrog
  +-- configuration values and work buffers
```

`ParticleSourceBuffer`, solver configuration, ownership IDs, packet buffers, and
transaction state are not grouped because they have different mutation
phases, rollback semantics, or owners. `SolverForceEvaluation` is the
non-owning per-step contract consumed by KDK. `SolverCpuForceProvider`,
`SimBackendForceProvider`, and `SimDistributedForceProvider` adapt that
contract to solver capabilities, optional backends, and communication without
exposing those concerns to the integrator. `std::variant` remains confined to
the composition boundary.

## Regression Contract

The refactor preserves the existing lifecycle, numerical, allocation, MPI,
rollback, overlap, HIP-fallback, package, and public ABI tests. No new owner
is introduced for an existing allocation, and no public type exposes either
composition object.
