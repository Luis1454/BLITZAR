# BLITZAR Clean-Room Plan

Status: **FROZEN**  
Product/API version: **1.0.0**
Plan version: **1.0.43**

This repository is a clean-room rewrite. The old repository, its source tree,
its issues, and its documentation are not implementation inputs. Requirements
are re-derived here as contracts, numerical references, and acceptance tests.

The authoritative planning state is the combination of this file,
`plan/manifest.json`, `plan/quality.json`, `plan/decision-index.json`, and
`plan/scaling.json`, `plan/layout.json`, `plan/output_contract.json`, and
`plan/final_audit.json`.
Decision records under `plan/decisions/` preserve the rationale and migration
history for that state.

## Product Boundary

BLITZAR is a deterministic particle-simulation library with an optional CUDA
backend. The public product has three layers:

1. A small C ABI in `include/blitzar/blitzar.h` for FFI consumers.
2. A C++20 RAII wrapper in `include/blitzar/blitzar.hpp`.
3. A standalone CLI in `apps/blitzar` using the same SDK as external users.

The simulation core owns particle state, integration, spatial data structures,
solvers, optional grids, and binary snapshot I/O. Networking, GUI code, and plugin
loading remain adapters; optional distributed execution is isolated under
`src/mpi` and must preserve the single-rank contract.

## Capability Contract

The public capability report is a compile-time contract, not a hardware
qualification result. `blitzar_get_capabilities_v2` reports which solver
contracts are implemented, explicitly unsupported, or deferred, together with
the optional backend code compiled into the library. It does not claim that a
GPU is visible or that MPI is running on more than one host.

Direct, Barnes-Hut, and CPU FMM are implemented solver contracts. PM and
TreePM remain explicit `BLITZAR_STATUS_UNSUPPORTED` selections and their
production roots remain deferred. The output contract is frozen in
`plan/output_contract.json`, and its logical versioned `SnapshotFrameView` plus
the single-rank binary codec are implemented and contract-qualified. The
single-rank run manifest, atomic output lifecycle, configured CLI
publication, human-readable and machine-readable production run summaries,
deterministic snapshot restart, online
conservation diagnostics, and snapshot post-processing are implemented and
locally qualified. MPI/HIP shard persistence and HDF5 remain deferred to their
dedicated issues.

HIP is capability-gated: compiler support and device execution are separate
conditions, and a CPU fallback is retained when no device is visible. MPI is
optional and locally qualified by rank-parity tests; real multi-node and RDMA
qualification remain unverified. CI records compile-only, skipped-device, and
executed-device outcomes separately.

## Reproducible Evidence

Performance and release claims require the workload contract in
`plan/scaling.json`. `tests/scaling/ScaleTest.cpp` is a measurement harness, not a
second simulation implementation: it exercises the existing `Sim`
execution path, records one result per MPI rank, and compares hierarchical
solvers with the Direct CPU oracle when the selected mode is CPU-qualified.

`tools/evidence/release_evidence.py` expands the strong-scaling, weak-scaling,
migration, and overlap workloads; `tools/evidence/release_evidence_test.py` verifies the
contract expansion, record parser, and benchmark metric invariants. The v2
contract names the deterministic `box-pair-v1` and `boundary-crossing-v1`
distributions, records wall time plus per-step timing, steady-state allocation
count, peak resident memory, and derived particle throughput, and requires every
rank record to identify its schema and distribution. It records the exact
command, Git revision, plan version, compiler/toolchain, operating system, CPU
capability, GPU capability, thread count, problem-size field, rank count,
backend, precision, seed, tolerances, memory result, communication volume,
overlap timeline, migration result, and raw rank output. Generated logs and
tables are written outside the source tree; only the contract and reporting
schema are versioned here. A skipped compiler, unavailable device, local
multi-rank run, and real multi-node run are distinct evidence states.
The `release-evidence` CI job builds the CPU/MPI probes and runs this matrix in
strict mode; its output is uploaded as an external artifact.

The layout qualification contract in `plan/layout.json` measures the shared
Morton ordering and bounded SoA/AoSoA candidates. `tests/layout` keeps the
current SoA representation selected at the Octree and future grid boundaries,
while `tools/evidence/layout_evidence.py` validates exact ordering, logical
state, Octree, and byte-repeatability hashes. Cache-line visits and contiguous
scan throughput are deterministic proxies, not hardware-counter claims; the
generated layout report is written outside the source tree.

## Final Qualification

`plan/final_audit.json` assigns an owner, category, and review gate to every
tracked repository path. `tools/audit/audit_final.py` materializes the complete file
matrix, hashes and scans each tracked file, checks CMake/source completeness,
validates the accepted architecture reviews and deferred capability register,
and resolves the recorded source commits or their squash-merge integration
commits for each completed issue. Integration commits are matched by the
recorded pull-request number before issue-style title markers. Its report,
finding register, and gate log
are generated outside the source tree.

The final CI job depends on every supported build, test, package, sanitizer,
debugger, static-analysis, MPI, and capability-gated GPU lane. A lane failure
blocks the final gate; an unavailable GPU remains a recorded skipped device
state, and no local multi-rank result is promoted to multi-node evidence.

## Repository Shape

```text
include/blitzar/                 Public ABI and C++ facade only
src/core/                         Stable internal contracts and value types
src/particles/                    Particle domain aggregator
src/particles/arena/              Aligned SoA arena ownership
src/particles/buffer/             Mutable particle and acceleration buffers
src/particles/source/              Remote source particle buffer
src/physics/gravity/             Force laws, units, softening, validation
src/integration/kdk/              Time integration and timestep policy
src/trees/octree/                 Bounded Octree storage, views, and resources
src/gpu/                          Optional GPU domain aggregator
src/gpu/runtime/                  HIP/CUDA runtime and launch policy
src/gpu/memory/                   Device and pinned-buffer ownership
src/gpu/direct/                   GPU direct-solver kernels
src/gpu/barnes_hut/               GPU Barnes-Hut kernels
src/mpi/                          Distributed domain aggregator
src/mpi/{collectives,domain,exchange,ghost,gather,packets,native,runtime}/ MPI responsibilities
src/grid/                        3D grids and mass deposition
src/solvers/                      Solver domain aggregator and shared contracts
src/solvers/threading/            Bounded traversal resources
src/solvers/direct/               O(N^2) CPU reference responsibilities
src/solvers/barnes_hut/           Barnes-Hut tree orchestration
src/solvers/fmm/                  FMM CPU responsibilities
src/solvers/pm/                  Particle-Mesh CPU and CUDA
src/solvers/treepm/              TreePM composition and dispatch
src/io/                          Binary snapshots and optional HDF5 adapter
src/io/snapshot/                 Single-rank binary snapshot codec
src/io/metadata/                 Deterministic run manifest and output lifecycle
src/sdk/{c,cpp}/                 Internal C ABI and C++ facade adapters
src/simulation/                   Simulation behavior aggregator and Sim facade
src/simulation/config/            Directive parsing and semantic configuration
src/simulation/initialization/    Configuration-derived deterministic initial state
src/simulation/staging/           Temporary caller-particle staging before commit
src/simulation/{runtime,solver,state,step,transaction}/ Simulation responsibilities
apps/blitzar/                    CLI executable; never library production code
tests/{contracts,fixtures,fmm,gpu,integration,mpi,octree,package,scaling,simulation}/ Tests by responsibility
tests/layout/                     Morton ordering and bounded layout qualification
examples/                        Minimal C and C++ SDK consumers
plan/                            Frozen roadmap and machine-readable invariants
tools/{architecture,gates,format,debug,evidence,audit}/ Repository policy checks
```

`src/grid`, `src/solvers/pm`, and `src/solvers/treepm` are planned but not
materialized roots. They remain in the deferred-root set until their production
ownership and tests exist. Binary snapshot and single-rank metadata ownership
are materialized under `src/io`; the optional HDF5 adapter remains deferred. The CPU FMM root is materialized in P3
with deterministic order-2 multipole qualification; GPU FMM remains outside
the current backend scope. The GPU runtime and native CUDA compatibility are
owned by `src/gpu`; HIP kernels are co-located below `gpu/{direct,barnes_hut}`,
while CPU solver implementations remain under `src/solvers`.

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
- P5 remains deferred because its production roots are not materialized. P6 is
  implemented locally in stages: its versioned frame contract, single-rank
  binary codec, deterministic manifest, atomic output lifecycle, and configured
  CLI publication, the production run summary, deterministic snapshot restart,
  online conservation diagnostics, and snapshot post-processing are qualified,
  while optional adapters remain open.
  P6 does not wait for the unrelated PM/TreePM
  implementation in P5.
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

The internal configuration reader accepts the legacy directive-file syntax as a
format contract only: one `directive(key=value, ...)` record per line, `#`
comments, quoted or unquoted values, and repeated directives in source order.
It preserves directive data without importing legacy scene generation, solver,
rendering, or GUI behavior. Applying any parsed value to the rewrite remains a
separate semantic contract.

The executable configuration contract supports only `simulation`, `gravity`,
`units`, and `generation` directives, with optional `barnes_hut`, `run`, `output`,
`diagnostics`, and `restart` directives. It creates rewrite-native deterministic
SoA input and executes the
public C++ facade. Unknown directives, malformed arguments, non-deterministic
generation, and deferred solver families are rejected rather than interpreted
through legacy behavior. The internal CLI composition target is kept on the
static-library path; shared-library qualification covers only public consumers.

### P3: Spatial Structures and Hierarchical Solvers

Implement Morton ordering, octree construction, multipoles, and Barnes-Hut.
Then implement FMM behind the same solver contract. Compare both against the
direct CPU oracle for force, energy, conservation, and deterministic ordering.
The orthogonal `SolverResourceContract` freezes the resource shape for Direct,
Barnes-Hut, FMM, KIFMM, PM, and TreePM. `OctreeView` exposes read-only
topology, geometry, stable ordering, and cell ranges with generation and
capacity validation. `OctreeResource` owns bounded preparation, and
`SolverTreeResources` composes local and remote tree resources for hierarchical
solver bundles. Direct contains no tree resource; PM and TreePM remain deferred
until a Grid root is materialized. `TST-P3-003` covers the matrix and
`TST-P3-004` covers resource lifetime and view invalidation. The
`SolverForceEvaluation` is the only force request consumed by KDK. Typed
`SolverForceRequest::Direct` and `SolverForceRequest::Tree` values are created
by the CPU provider traits, while backend and distributed providers keep
capability and communication policy outside the integrator. `std::variant` is
visited only at the simulation composition boundary. `TST-P3-005` covers the
typed provider contract, local and remote CPU evaluation, and the preserved
direct force semantics.

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

The output contract is frozen before the `src/io` root is materialized. The
logical frame descriptor, single-rank binary reader/writer, deterministic run
manifest, and atomic snapshot publication lifecycle are implemented and
qualified by their P6 tests. The reusable conservation metrics module and its
KDK requalification are now covered by `TST-P6-005`. The production CLI
summary formats are covered by `TST-P6-006`. The CLI defaults to a stable
labeled human presentation; automation selects the byte-stable JSON format
with `--format json`. Deterministic snapshot restart is covered by `TST-P6-008`.
Restart payload validation and production executable entrypoint
coverage are covered by `TST-P6-009`. Online conservation diagnostics and
snapshot post-processing, including byte parity with the online CSV, are
covered by `TST-P6-010`. Output boundary qualification is covered by
`TST-P6-011`: a single-rank MPI launch is byte-equivalent to the direct CPU CLI
path, while multi-rank output is rejected before state capture or manifest
preparation. MPI/HIP shard persistence and the optional HDF5 adapter remain open.
The complete run pipeline remains unfinished; HDF5 remains deferred until its
executable evidence exists.

### P6 output boundary qualification

The CLI creates an internal MPI context only when configured output is enabled.
If that context observes more than one rank, the run returns
`BLITZAR_STATUS_UNSUPPORTED` before initialization, state capture, or output
preparation. This prevents the distributed `Sim::GetState` gather operation from
being used implicitly by persistence; that operation remains available to the
internal MPI qualification tests.

`TST-P6-004` records separate steady-clock measurements for physics steps and
output checkpoints. The physics interval contains only `Simulation::step`, and
the output interval contains state capture plus snapshot/diagnostic publication.
These values are qualification instrumentation only: they are not persisted in
manifests, snapshots, or the public summary. HIP device execution remains
capability-gated; MPI output qualification covers one host and explicitly does
not claim multi-node or RDMA behavior.

### Sprint 6: Optional HIP Acceleration

The HIP backend is detected in `AUTO` mode and can be forced with
`BLITZAR_HIP_MODE=ON`. CPU-only builds must not require ROCm, HIP headers, or a
GPU. The acceptance contract compares HIP Direct and Barnes-Hut forces with the
CPU reference when a device is available, while the same test validates the
unsupported/fallback path otherwise.

### Sprint 6.1: Native NVIDIA CUDA Compatibility

When `HIP_PLATFORM=nvidia` is explicit, CMake may use the CUDA language and
`nvcc` without requiring `hipcc` or HIP headers. `src/gpu/runtime/GpuCompatibility.hpp`
provides only the runtime calls used by the kernels; the kernels remain single
`.hip` sources and the compatibility layer stays internal. AMD continues to
use the ROCm HIP path, while an unselected or unavailable backend keeps the
CPU fallback.

P4 qualification uses the same `blitzar_accelerator_test` contract on Linux and
Windows. The Windows lane is one capability-gated matrix with native NVIDIA
CUDA and AMD HIP entries; each entry records its compiler and device probe
before configuring the backend. Missing toolchains or devices are explicit
skips, not successful hardware claims.

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
