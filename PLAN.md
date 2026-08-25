# BLITZAR Clean-Room Plan

Status: **FROZEN**  
Product/API version: **1.0.0**
Plan version: **1.0.13**

This repository is a clean-room rewrite. The old repository, its source tree,
its issues, and its documentation are not implementation inputs. Requirements
are re-derived here as contracts, numerical references, and acceptance tests.

The authoritative planning state is the combination of this file,
`plan/manifest.json`, `plan/quality.json`, and `plan/decision-index.json`.
Decision records under `plan/decisions/` preserve the rationale and migration
history for that state.

## Product Boundary

BLITZAR is a deterministic particle-simulation library with an optional CUDA
backend. The public product has three layers:

1. A small C ABI in `include/blitzar/blitzar.h` for FFI consumers.
2. A C++20 RAII wrapper in `include/blitzar/blitzar.hpp`.
3. A standalone CLI in `apps/blitzar` using the same SDK as external users.

The simulation core owns particle state, integration, spatial data structures,
solvers, optional grids, and snapshot I/O. Networking, GUI code, and plugin
loading remain adapters; optional distributed execution is isolated under
`src/parallel` and must preserve the single-rank contract.

## Repository Shape

```text
include/blitzar/                 Public ABI and C++ facade only
src/core/                        Stable internal contracts and value types
src/particles/                   Aligned SoA particle storage and invariants
src/physics/                     Force laws, units, softening, validation
src/integration/                 Time integration and timestep policy
src/trees/                       Morton ordering, octree, multipoles
src/gpu/                         HIP/CUDA runtime, pinned staging, streams, and launch policy
src/parallel/                    MPI context, domain ownership, and exchange
src/grid/                        3D grids and mass deposition
src/solvers/direct/              O(N^2) CPU reference and HIP acceleration
src/solvers/barnes_hut/          O(N log N) CPU and HIP
src/solvers/gpu/                 HIP kernel launch contracts and implementations
src/solvers/fmm/                 FMM CPU
src/solvers/pm/                  Particle-Mesh CPU and CUDA
src/solvers/treepm/              TreePM composition and dispatch
src/io/                          Binary snapshots and optional HDF5 adapter
src/sdk/                         Internal implementation of the public SDK
apps/blitzar/                    CLI executable; never library production code
tests/                           Unit, reference, contract, and integration tests
examples/                        Minimal C and C++ SDK consumers
plan/                            Frozen roadmap and machine-readable invariants
tools/                           Repository policy checks only
```

`src/grid`, `src/io`, `src/solvers/pm`, and `src/solvers/treepm` are planned
but not materialized roots. They remain in the deferred-root set until their
production ownership and tests exist. The CPU FMM root is materialized in P3
with deterministic order-2 multipole qualification; GPU FMM remains outside
the current backend scope. The
GPU runtime and native CUDA compatibility are intentionally owned by
`src/gpu`; no parallel CUDA runtime root exists.

Build trees are not repository content. Local builds use `../.blitzar-build`
by default, and CI builds use `${RUNNER_TEMP}/blitzar-build`; the versioned
workspace policy in `plan/workspace.json` rejects generated files if they ever
become tracked. Existing ignored `build*` directories are inventory data and
can be removed independently without changing source ownership.

Each production module owns its headers, implementations, and responsibility
subdirectories. There are no generic `utils`, `common`, `misc`, `private`, or
`details` catch-all directories. C++ and CUDA filenames are PascalCase, short,
and unique as complete names including their extensions. A matching `.cpp`/`.hpp`
stem is allowed when the two files implement the same primary type. Directory
names describe the domain.

## Delivery Order

The phases are ordered dependencies, not a list of parallel experiments.

### Phase status

- P0, P1, P2, and P3 are implemented and locally qualified.
- P4 is implemented with capability-gated GPU execution evidence.
- P5 and P6 remain deferred because their production roots are not materialized.
- P7 and P8 are implemented and locally qualified from the P3/P4 contracts;
  they do not depend on the deferred P5/P6 roots.

### P0: Contracts and Build Skeleton

Define scalar types, units, error model, deterministic execution settings,
solver interfaces, snapshot versioning, and the C ABI handle ownership rules.
Create the CMake targets and test harness. No solver optimization is allowed
before the contracts compile and the empty SDK lifecycle is tested.

### P1: Particle Core and Direct Reference

Implement aligned structure-of-arrays storage, validation, host ownership, the
Plummer softening law, fixed-step Leapfrog KDK, and the CPU direct solver.
The direct CPU solver is the numerical oracle for later implementations. Every
small reference case must have deterministic positions, velocities, energy,
momentum, and error tolerances.

### P2: Public SDK and CLI

Expose only the stable C ABI and the C++ RAII facade. Implement the CLI as a
consumer of that facade, not as a second execution path. Add C and C++ examples
that build against installed public headers only.

### P3: Spatial Structures and Hierarchical Solvers

Implement Morton ordering, octree construction, multipoles, and Barnes-Hut.
Then implement FMM behind the same solver contract. Compare both against the
direct CPU oracle for force, energy, conservation, and deterministic ordering.

### P4: HIP Runtime and GPU Solvers

Add an optional HIP runtime context with explicit ownership for device buffers,
pinned host staging, streams, and synchronization. Port the direct solver first,
then Barnes-Hut through flat octree views. HIP headers remain confined to `.hip`
implementations; the C++ SDK receives no GPU type. If HIP or a device is absent,
the selected OpenMP CPU solver remains the execution path.

### P5: Grid, PM, and TreePM

Implement grid layout, mass deposition, boundary conditions, and the PM solver.
TreePM composes the tree and grid contracts; it does not duplicate either
implementation. CPU behavior is qualified before CUDA dispatch is enabled.

### P6: Persistence and Qualification

Add versioned binary snapshots and the optional HDF5 adapter. Validate corrupt,
truncated, incompatible, and endian-swapped inputs. Add performance baselines,
long-run determinism checks, and CPU/GPU parity reports.

### Sprint 6: Optional HIP Acceleration

The HIP backend is detected in `AUTO` mode and can be forced with
`BLITZAR_HIP_MODE=ON`. CPU-only builds must not require ROCm, HIP headers, or a
GPU. The acceptance contract compares HIP Direct and Barnes-Hut forces with the
CPU reference when a device is available, while the same test validates the
unsupported/fallback path otherwise.

### Sprint 6.1: Native NVIDIA CUDA Compatibility

When `HIP_PLATFORM=nvidia` is explicit, CMake may use the CUDA language and
`nvcc` without requiring `hipcc` or HIP headers. `src/gpu/HipCompat.hpp`
provides only the runtime calls used by the kernels; the kernels remain single
`.hip` sources and the compatibility layer stays internal. AMD continues to
use the ROCm HIP path, while an unselected or unavailable backend keeps the
CPU fallback.

### Sprint 7: Optional MPI Domain Decomposition

When `BLITZAR_MPI_MODE=ON` and MPI CXX is available, the internal parallel
adapter initializes MPI with `MPI_THREAD_MULTIPLE`, partitions global particle
IDs by deterministic Morton-key slices, and keeps MPI types out of the public
SDK. `AUTO` and `OFF` preserve the single-rank Sprint 6.1 path.

The full input state is accepted and staged only by rank zero. Other ranks may
pass an empty view; they receive their deterministic Morton slice through a
bounded packet exchange and own only their local arena prefix. At each
completed KDK step, ownership migration uses a contiguous `ParticlePacket`
packing buffer and `MPI_Alltoallv`. Before each force evaluation, non-blocking
`MPI_Isend`/`MPI_Irecv` exchanges all remote packets required for exact
long-range gravity; this conservative halo is intentional because a truncated
boundary halo would change the gravitational result. `GetState` gathers packets
by stable global ID. The acceptance tests `TST-P7-001` and `TST-P7-002` run the
same deterministic case with two and four ranks and compare it to the direct
single-rank reference within `1e-5`.

### P7 extension: Distributed Input Ownership and Bounded State

Persistent MPI state is rank-local. `SetParticles` keeps the root-only input
stage and temporary distribution exchange alive only for the initialization
transaction; non-root ranks never allocate or fill a global `ParticleArena`.
Ghosts are stored in a separate source-only SoA and packet/wire buffers grow
only when an observed halo or gather requires them. The Barnes-Hut primary tree
uses the local rank capacity, while the remote tree is lazy and reserved only
for a distributed force evaluation. The MPI test reports peak RSS per rank
with particle count and rank count so memory can be compared across `P=1`,
`P=2`, and `P=4` runs.

The root-only input contract, two/four-rank parity, Barnes-Hut migration, and
rollback behavior are covered by `TST-P7-001`, `TST-P7-002`, and
`TST-P7-004`.

### P8: MPI Boundary and KDK Overlap

`MpiContext` is the only module that knows MPI types and collectives. Its
opaque asynchronous exchange handle lets `MpiExchange` retain only packet
packing and ownership layout. The single-rank fallback uses the same context
contract without compiling MPI headers in the decomposition or exchange code.

Each KDK force phase starts the non-blocking halo exchange before computing the
local contribution. The Direct solver computes local-source forces first and
adds the remote-source contribution after completion; Barnes-Hut performs its
local tree work while the exchange is pending, then rebuilds the complete tree
for the final force. Ownership migration is committed immediately after Drift
through a KDK transition hook, and the KDK checkpoint is recaptured for
the new local prefix before the second force evaluation. `TST-P8-001` forces
inter-rank movement and compares the result with the direct single-rank oracle.

## Non-Goals for the Initial Rewrite

- Reusing or mechanically translating old implementation files.
- Recreating legacy server, RPC, GUI, plugin, or distributed-runtime code.
- Enabling adaptive timesteps before fixed-step KDK is qualified.
- Treating CUDA as the numerical ground truth.
- Adding a family of nearly identical configuration or state structs.

Deferred features may be proposed only as a new plan change with a contract,
an owner, an oracle, and an acceptance test.

## Architecture Gates

Every module must have one clear responsibility, explicit ownership, and a
small contract boundary. Prefer composition and constructor dependency
injection. No singleton, owning raw pointer, hidden global state, or public
mutable aggregate state is allowed.

The quality gate evaluates function length, function count, branching
complexity, allocation behavior, include dependencies, and test evidence. Line
count alone never decides a split. A struct is split when it owns unrelated
lifecycle, policy, or data domains, not merely because it has many fields.

Every optimization must preserve the direct CPU reference within a declared
tolerance. Every public behavior must be testable without the CLI. Every
automated test must have a stable identifier in the quality manifest once that
manifest is introduced in P0.

## Change Protocol

This plan is frozen. A pull request changing `PLAN.md` or `plan/manifest.json`
must:

1. Use a commit message beginning with `plan-change:`.
2. Add or update a decision record under `plan/decisions/` and its lifecycle
   entry in `plan/decision-index.json`.
3. Update the plan version and affected phase identifiers.
4. Explain migration impact and add acceptance tests.

The GitHub Actions plan gate rejects all other plan changes.
