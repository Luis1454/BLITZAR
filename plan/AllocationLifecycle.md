# Allocation Lifecycle Review

Issue: #607

The execution contract is allocation-free after construction and explicit
configuration/preparation. The allocation monitor counts host `new` calls in
the test process; it does not claim device allocator evidence when no HIP
device is available.

## Phase Matrix

| Allocation owner | Allocation site | Allowed phase | Steady-state rule |
| --- | --- | --- | --- |
| `Simulation` | runtime, particle storage, source, solver, IDs, packet buffers | construction | all capacities are fixed before the first step |
| `Simulation` | `gathered_buffer_` | construction | `GetState` only resizes within its reserved capacity |
| `ParticleArena` and `SourceBuffer` | aligned SoA replacement storage | construction or explicit capacity preparation | execution returns a bounded status instead of growing storage |
| solver variant | solver staging, octree, multipoles, remote tree, thread stacks | solver construction or configuration rebuild | `Prepare` may validate capacity but must not grow after the configured bound |
| `MpiContext` | packet wire buffers, ghost wire buffers, request vectors | MPI context preparation | ghost, migration, gather, and all-to-all paths reuse these buffers |
| `StepTransaction` | arena, force, and exchange snapshots | simulation construction | `Prepare` and `Abort` only resize within reserved buffers |
| C ABI and C++ wrapper handles | opaque implementation objects | API construction/configuration | never allocated by `Step` or `GetState` |
| HIP context and device buffers | runtime/device allocations | HIP initialization and solver preparation | kernels and fallback dispatch do not create host workspaces |
| input staging and candidate solvers | temporary vectors and replacement variants | `SetParticles` or configuration mutators | outside the step transaction and committed only after validation |

## Overflow Contract

Execution-facing stores reject a request larger than their prepared capacity
before changing count or payload state. In particular, ghost storage no
longer calls `SourceBuffer::Reserve` from `StoreGhosts`; the caller must
prepare the source capacity before the exchange begins.

## Evidence Matrix

| Mode | Evidence |
| --- | --- |
| CPU Direct, Barnes-Hut, FMM | local allocation test after warmup |
| MPI Direct, Barnes-Hut, FMM | MPI allocation test after warmup and gather |
| MPI migration | repeated migration step plus gather under the monitor |
| MPI rollback | failed KDK step, transaction abort, and state read under the monitor |
| MPI overlap | overlapped and serialized steps plus gather under the monitor |
| HIP fallback | CPU allocation test exercises the unavailable-backend fallback |
| HIP device | requires a HIP-capable runner; host evidence alone is not sufficient |

The monitor is started only after construction, configuration, input staging,
solver preparation, and one warmup step. A zero count therefore qualifies the
steady-state path, not the intentionally allocating setup path.
