# BLITZAR Clean-Room Plan

Status: **FROZEN**  
Product/API version: **1.0.0**
Plan version: **1.0.53**

This repository is a clean-room rewrite. The old repository, its source tree,
its issues, and its documentation are not implementation inputs. Requirements
are re-derived here as contracts, numerical references, and acceptance tests.

The authoritative planning state is the combination of this file,
`plan/manifest.json`, `plan/quality.json`, `plan/decision-index.json`, and
`plan/scaling.json`, `plan/layout.json`, `plan/reduction.json`,
`plan/neighborhood.json`, `plan/output_contract.json`, `plan/delta.json`, and
`plan/block_time.json`, `plan/bvh.json`, `plan/grid.json`, and
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

Direct, Barnes-Hut, CPU FMM, PM, and TreePM are implemented CPU solver
contracts. PM and TreePM are qualified for single-rank execution; their
distributed mesh path remains explicitly unsupported until its ownership and
halo contract is versioned. The output contract is frozen in
`plan/output_contract.json`; the HDF5 and delta policies are frozen in
`plan/hdf5.json` and `plan/delta.json`. The logical versioned
`SnapshotFrameView` and the single-rank binary codec are implemented and
contract-qualified. The
single-rank run manifest, atomic output lifecycle, configured CLI
publication, human-readable and machine-readable production run summaries,
deterministic snapshot restart, online
conservation diagnostics, and snapshot post-processing are implemented and
locally qualified. HDF5 CLI output is implemented as an optional
capability-gated format. Explicit MPI/HIP rank-shard persistence is implemented
for the local qualification path; HDF5 remains capability-gated when its
optional dependency is absent, and multi-node/RDMA qualification remains
unverified.

The block-time qualification contract is frozen in `plan/block_time.json`.
Its bounded scheduler model is evidence only: fixed-step KDK remains the
production integrator, and the reported work reduction is not a physical or
end-to-end speedup claim.

The BVH qualification contract is frozen in `plan/bvh.json`. It measures an
iterative AABB index for irregular local queries against the exact directed
neighbor reference, the selected cell-linked candidate, and a separate Octree
baseline. BVH refit and full rebuild produce the same query results, but the
candidate is not promoted into the production long-range gravity path.

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

The reduction qualification contract in `plan/reduction.json` measures Plain,
Kahan, and Neumaier over fixed cancellation workloads and a 4096-step KDK
trajectory. `ScalarReduction` keeps the Direct CPU force oracle on ordered
plain addition while `ConservationMetrics` selects Neumaier for diagnostic
energy and momentum. The reduction evidence runner validates error, timing,
ordering hashes, finite results, and exact default-policy equivalence; reports
are written outside the source tree.

The local-neighbor qualification contract in `plan/neighborhood.json` compares
cell-linked buckets, deterministic spatial hashing, 3D Hilbert ordering, and
Verlet lists over dense, sparse, clustered, and moving workloads. Its exact
O(N^2) reference preserves original target/source ordering and its strict
runner reports rebuild schedule, memory, and separate Octree baseline metrics.
The cell-linked list is selected for future local interactions; no local index
is promoted into production by this qualification, and the Octree remains a
long-range structure rather than a local-neighbor oracle.

The block-time qualification contract in `plan/block_time.json` compares the
fixed KDK schedule with a deterministic power-of-two time-bin schedule over
heterogeneous, clustered, and migration workloads. `TST-P1-008` checks stable
active ordering, synchronization-boundary ownership changes, ledger
conservation, restart, rollback, and input non-mutation. Its modeled speedup
is fixed event count divided by block event count, and its timing covers only
the scheduler loop. The candidate remains `not-selected`: no asynchronous
force state, MPI transport change, snapshot ABI change, or public SDK change
is introduced until a complete end-to-end implementation is qualified.

The BVH qualification contract in `plan/bvh.json` compares dense, sparse,
clustered, and moving local-query workloads. `TST-P1-009` checks deterministic
AABB construction, iterative traversal, exact neighbor parity, stable output
ordering, refit/rebuild parity, memory, and wall-clock measurements. The
Octree remains a separate long-range baseline and the cell-linked structure
remains the selected local-neighbor policy; no physical force or production
gravity speedup is claimed.

The P5 grid contract in `plan/grid.json` qualifies the materialized finite
8³ Grid resource, cloud-in-cell deposition, softened discrete Green
convolution, and clamped trilinear interpolation. `TST-P5-001` checks grid
layout and mass conservation, `TST-P5-002` compares PM with the Direct CPU
reference, and `TST-P5-003` checks TreePM composition. The PM and TreePM CPU
paths are single-rank and allocation-free after preparation; no FFT, GPU
speedup, or distributed mesh claim is made.

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
src/physics/reduction/            Ordered plain and compensated scalar reductions
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
tests/reduction/                  Compensated reduction and conservation qualification
tests/neighborhood/                Local-neighbor candidate and exact-reference qualification
tests/bvh/                         AABB BVH irregular-query qualification
tests/pm/                           Grid and PM CPU qualification
tests/treepm/                       TreePM composition qualification
examples/                        Minimal C and C++ SDK consumers
plan/                            Frozen roadmap and machine-readable invariants
tools/{architecture,gates,format,debug,evidence,audit}/ Repository policy checks
```

`src/grid`, `src/solvers/pm`, and `src/solvers/treepm` are materialized P5
production roots. Their ownership and acceptance tests are frozen in
`plan/grid.json` and Decision 066. Binary snapshot and single-rank metadata ownership
are materialized under `src/io`; the optional HDF5 adapter and CLI format
selection are capability-gated in `src/io/hdf5`. The CPU FMM root is materialized in P3
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

- P0, P1, P2, P3, and P5 are implemented and locally qualified.
- P4 is implemented with capability-gated GPU execution evidence.
- P6 is implemented locally in stages: its versioned frame contract, single-rank
  binary codec, deterministic manifest, atomic output lifecycle, and configured
  CLI publication, the production run summary, deterministic snapshot restart,
  online conservation diagnostics, and snapshot post-processing are qualified,
  while the internal HDF5 adapter and CLI HDF5 output are capability-gated.
  P6 does not depend on the PM/TreePM runtime path in P5.
- P7 and P8 are implemented and locally qualified from the P3/P4 contracts;
  their existing distributed qualification does not promote the single-rank
  P5 mesh path into MPI.
- The block-time scheduling candidate is qualified as a P1 evidence boundary,
  but it is not promoted into production. Any physical block integrator must
  be a later plan change with force-state, MPI, restart, rollback, and
  end-to-end performance evidence.

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

The force reference retains ordered plain accumulation. Compensated policies
are isolated to the diagnostic reduction boundary: Neumaier is the selected
default for conservation energy and momentum, while Kahan remains a measured
alternative. `TST-P1-006` verifies cancellation error, throughput,
repeatability, vectorization eligibility, and long-run diagnostic behavior
without changing the Direct CPU trajectory.

`TST-P1-007` qualifies local-neighbor candidates independently from long-range
solver execution. It fixes the finite-box, radius, skin, ownership, ordering,
rebuild, and memory contracts used by local interactions; it does not replace
the P5 long-range Grid resource.

`TST-P1-008` qualifies a bounded block-time schedule without changing the
fixed-step KDK path. It compares deterministic event ledgers, verifies that
ownership migration occurs only at a synchronization boundary, and checks
exact restart and rollback replay. The result is a scheduling proxy and does
not establish physical conservation or end-to-end simulation speedup.

`TST-P1-009` qualifies an AABB BVH only for irregular local queries. It uses
an explicit stack, deterministic median splits, stable source ordering, and a
bottom-up refit policy, then compares both refit and full-rebuild passes with
the exact neighbor reference and selected cell-linked index. The existing
Octree is reported separately as the long-range baseline; the BVH is not a
production gravity replacement.

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
generation, and unsupported distributed mesh execution are rejected rather than interpreted
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
solver bundles. Direct contains no tree resource; PM owns a Grid resource and
TreePM composes Octree and Grid resources. `TST-P3-003` covers the matrix and
`TST-P3-004` covers resource lifetime and view invalidation. The
`SolverForceEvaluation` is the only force request consumed by KDK. Typed
`SolverForceRequest::Direct` and `SolverForceRequest::Tree` values are created
by the CPU provider traits, while backend and distributed providers keep
capability and communication policy outside the integrator. `std::variant` is
visited only at the simulation composition boundary. `TST-P3-005` covers the
typed provider contract, local and remote CPU evaluation, and the preserved
direct force semantics.

The CPU KIFMM extension reuses the read-only Octree view and owns a bounded
equivalent/check-surface workspace. Its deterministic interaction list uses the
same acceptance criterion as FMM: near leaves are evaluated by the Direct CPU
kernel, while accepted cells use a kernel-independent equivalent representation
and a depth-indexed translation operator. KIFMM remains an internal solver until
its independent accuracy, rollback, and capacity qualification is complete;
the public ABI, CLI, and GPU/MPI paths are unchanged by this extension.

### P4: HIP Runtime and GPU Solvers

Add an optional HIP runtime context with explicit ownership for device buffers,
pinned host staging, streams, and synchronization. Port the direct solver first,
then Barnes-Hut through flat octree views. HIP headers remain confined to `.hip`
implementations; the C++ SDK receives no GPU type. If HIP or a device is absent,
the selected OpenMP CPU solver remains the execution path.

### P5: Grid, PM, and TreePM

Materialize the bounded Grid resource with finite non-periodic clamped
boundaries, cloud-in-cell mass deposition, and deterministic softened discrete
Green convolution. PM owns the Grid resource and exposes the typed CPU force
request through the existing provider contract. TreePM composes PM with the
existing Barnes-Hut resource and stages both fields before a weighted commit;
it does not duplicate either implementation. The default CPU mesh is 8³ with
an input-derived domain and one-unit padding. Single-rank CPU behavior is
qualified against Direct before any accelerator dispatch; distributed PM and
TreePM reject during step preflight until a global mesh ownership contract is
added.

### P6: Persistence and Qualification

The output contract is frozen before the `src/io` root is materialized. The
logical frame descriptor, single-rank binary reader/writer, deterministic run
manifest, and atomic snapshot publication lifecycle are implemented and
qualified by their P6 tests. The reusable conservation metrics module and its
KDK requalification are now covered by `TST-P6-005`; its ordered diagnostic
reductions use the Neumaier policy selected by Decision 060. The production CLI
summary formats are covered by `TST-P6-006`. The CLI defaults to a stable
labeled human presentation; automation selects the byte-stable JSON format
with `--format json`. Deterministic snapshot restart is covered by `TST-P6-008`.
Restart payload validation and production executable entrypoint
coverage are covered by `TST-P6-009`. Online conservation diagnostics and
snapshot post-processing, including byte parity with the online CSV, are
covered by `TST-P6-010`. Output boundary qualification is covered by
`TST-P6-011`: a single-rank MPI launch is byte-equivalent to the direct CPU CLI
path. Explicit MPI/HIP rank shards are covered by `TST-P7-006`: each rank writes
one local state file, the root manifest is published only after all shard writes
complete, and no distributed output path invokes the `GetState` gather. The
internal HDF5 adapter
and its CLI format selection are capability-gated under `src/io/hdf5`; they are
covered by `TST-P6-012` and `TST-P6-014` when HDF5 is available, while the
unavailable path is explicitly tested. HDF5 output does not change the public
ABI, and restart/post-processing select the source manifest format.

### P6 optional HDF5 adapter qualification

The adapter maps the frozen `SnapshotFrameView` to one versioned `/particles`
group with eight SoA datasets and canonical little-endian FNV-1a-64 payload
validation. It is selected by `BLITZAR_HDF5_MODE=AUTO|ON|OFF`, publishes through
an atomic temporary file, and reads through bounded staging before mutating the
caller state. HDF5 headers remain below `src/io/hdf5`; the public C and C++ SDK
headers remain binary-only. An unavailable dependency falls back to the binary
codec or returns `BLITZAR_STATUS_UNSUPPORTED` for the optional adapter. The
round-trip, corruption, truncation, restart, repeated-write, package, and
unavailable-dependency evidence is registered by `TST-P6-012`, `CHK-P0-038`,
`CHK-P0-039`, and the package consumer test. Decision 067 extends that adapter
through the configured CLI without changing the binary default.

### P6 snapshot delta qualification

`SnapshotDelta` is an internal, evaluated-not-default payload codec shared by
snapshot and transport boundaries. It applies deterministic XOR followed by
bounded zero/literal runs to the canonical little-endian SoA payload. Its
24-byte stream header records the raw payload size and FNV-1a-64 checksum. All
buffers are supplied by the caller, so steady-state encode and decode do not
allocate and failed decode validation cannot mutate the destination.

Delta frames use an explicit keyframe interval of eight. An index maps a step
to its nearest keyframe and replays at most seven deltas; the existing binary
snapshot remains the direct random-access restart fallback. `TST-P6-013`,
`CHK-P0-040`, and `CHK-P0-041` measure the ratio, CPU cost, workspace size, and
in-memory read/write latency while proving deterministic bytes, checksum and
corruption rejection. Filesystem latency and numerical state representation
are outside this candidate evaluation, and the public ABI is unchanged.

### P6 output boundary qualification

The CLI creates an internal MPI context only when configured output is enabled.
If that context observes more than one rank, the run uses the explicit shard
contract in Decision 068. Local state capture is non-collective, every rank
publishes an atomic rank-local file, and the root manifest is updated only after
collective status confirmation. The distributed `Sim::GetState` gather remains
available to internal numerical qualification, but is not used by persistence.

`TST-P6-004` records separate steady-clock measurements for physics steps and
output checkpoints. The physics interval contains only `Simulation::step`, and
the output interval contains state capture plus snapshot/diagnostic publication.
These values are qualification instrumentation only: they are not persisted in
manifests, snapshots, or the public summary. HIP device execution remains
capability-gated; shard output qualification covers one host and explicitly does
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

Distributed persistence uses the same root-only input and local ownership
boundaries. Each checkpoint writes one strictly ID-ordered shard per rank and
publishes a root manifest only after all rank-local atomic writes succeed.
Restart requires the source rank count and reconstructs the global initialization
state explicitly on rank zero before the existing bounded distribution. The
post-process path enumerates and validates every shard without a hidden gather.
Online distributed diagnostics remain unsupported pending a versioned global
reduction contract. `TST-P7-006` covers the two-rank shard, rerun, and restart
acceptance path.

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
