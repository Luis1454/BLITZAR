# Repository State And Recovery Plan

Snapshot date: 2026-08-15.

This document records the integrated workspace checkpoint introduced by issue
#436. It distinguishes code present in the tree from evidence actually produced
on a local machine. It is not a release qualification record.

## Checkpoint Scope

The checkpoint includes engine, CUDA, runtime, Qt GUI, headless execution,
desktop packaging, CI, documentation, scripts, and tests. Before the checkpoint
there were 159 modified tracked files and 63 untracked files (223 total).

## Component Status

| Component | State | Evidence in this checkpoint | Follow-up |
| --- | --- | --- | --- |
| Build, desktop, Docker, CI | Integrated, not cross-platform qualified | CMake, presets, desktop launcher, CPU Dockerfile, macOS workflow, release scripts changed | #306, #307 and release issues |
| Headless and server execution | Integrated, not full E2E qualified | CLI, batch runner, server lifecycle, runtime protocol, export controls and tests changed | Run clean Windows/Linux/macOS smoke matrix |
| TreePM CPU/CUDA | Implemented, performance and accuracy not fully qualified | GPU reductions, grid pipeline, layouts, CUDA Graph/JIT paths, CPU support, benchmark tooling changed | #303 and #439 |
| Adaptive individual timestep | Implemented, broad regression pending | Hierarchical integration code, configuration and protocol tests changed | #72 and #439 |
| Comoving cosmology | Experimental implementation | Configuration, periodic PM path, cases, spectrum tooling and documentation changed | #438 |
| CPU FMM | Implemented and locally unit-tested | Adaptive tree, 2:1 balancing pass, order-2 Cartesian FMM, force/invariant metrics, five deterministic tests | #437 |
| Qt GUI and scene editor | Integrated, functional qualification incomplete | Scene/config editors, viewport, graphs, solver controls, non-intrusive UI tests and GUI scripts changed | #440 and #84 |
| Rendering and structure spectrum | Integrated, visual/scientific interpretation pending | GPU view, spectrum graph, analysis tooling, test and documentation changed | #438 and GUI qualification |
| Runtime/client protocol | Integrated, system-level qualification pending | Client bridge, FFI, protocol schema, daemon and integration tests changed | #86, #84 |

## Confirmed Local Evidence

The following commands completed on the Windows workspace before this
checkpoint:

```powershell
cmake --build build-gui-check --config RelWithDebInfo --target blitzarPhysicsGTests --parallel 2
.\build-gui-check\blitzarPhysicsGTests.exe --gtest_filter=PhysicsTest.TST_UNT_FMM_*
cmake --build build-gui-check --config RelWithDebInfo --target blitzar-headless --parallel 2
.\build-gui-check\blitzar-headless.exe --validate --config tests\data\fmm_cpu.ini
.\build-gui-check\blitzar-headless.exe --run --config tests\data\fmm_cpu.ini --target-steps 3 --no-export-on-exit
```

The FMM test filter passed five tests: pairwise comparison, Plummer sphere,
galaxy collision, perturbed cosmology, and Leapfrog invariant coverage. The
headless FMM run completed 512 particles and three steps with `faulted=0` on
the CPU backend. This does not establish large-N performance, CUDA FMM support,
or end-to-end cosmology validity.

Known build debt: the local CUDA/MSVC build emits existing `xutility` warnings.
This checkpoint does not claim the repository has a zero-warning build.

## Current CI Recovery

The consolidation PR has restored the Linux and Windows module builds. The
macOS build, Python coverage prerequisites, test catalog, and traceability
format are under active CI revalidation. The remaining policy failures are
historical structural violations and require scoped module decompositions; they
are not waived by this checkpoint. Independent IV&V approval remains an
external merge requirement.

## Physics Source Layout

The physics implementation is organized by responsibility rather than by build
backend alone:

- `engine/physics/core/include/`: shared particle, vector, state, and force-law contracts.
- `engine/physics/octree/include/`: octree data structure contract.
- `engine/physics/treepm/include/`: CPU TreePM contract.
- `engine/physics/fmm/include/`: CPU FMM contract.
- `engine/physics/cuda/include/`: CUDA/JIT-facing contracts.
- `engine/physics/core/src/`: shared host implementations.
- `engine/physics/octree/src/`: host octree and particle-system fallback implementation.
- `engine/physics/treepm/src/`: CPU TreePM implementation.
- `engine/physics/fmm/src/`: FMM construction, evaluation, and metrics.
- `engine/physics/cuda/src/`: host-side CUDA/JIT bridge implementation.
- `engine/physics/cuda/fragments/`: shared CUDA system, integration, and JIT fragments.
- `engine/physics/<method>/cuda/fragments/`: method-specific CUDA fragments.
- `engine/<domain>/<module>/Module.cmake`: explicit source and include manifest for each engine module.

New solver code belongs in its solver directory. Cross-solver state and numerical
contracts belong in `core`; CUDA fragments remain grouped by execution concern
under `physics/cuda/fragments/`.

## Issue Disposition

| Issue | Disposition | Rationale |
| --- | --- | --- |
| #424, #426-#431 | Close as outdated | Their planned `modules/qt/ui` and mirrored `include/ui` paths have been superseded by the current `modules/qt/src` layout. Their structural goals are restated against the active files by #440. |
| #289 | Closed as superseded | The obsolete `ParticleSystemUpdate.inl` path was replaced by the current CUDA timestep decomposition tracked by #467. |
| #303 | Keep open | GPU octree exactness and performance evidence remains incomplete. |
| #304 | Keep open | The checkpoint does not prove a formal VTK compatibility specification. |
| #306, #307 | Keep open | Cross-platform Rust/tooling and artifact-cleanup acceptance criteria have not been requalified. |
| #360, #375 | Keep open | Coverage and deterministic precision requirements are not complete. |
| #72, #84, #86 | Keep open | They remain valid umbrella and E2E follow-up work. |
| #437 | New | FMM scale, accuracy, invariant, allocation, and CUDA decisions. |
| #438 | New | Cosmology physical validation. |
| #439 | New | TreePM numerical and hardware qualification. |
| #440 | New | Active Qt module decomposition and non-intrusive tests. |

## Recovery Order

1. Merge this consolidation checkpoint only after the targeted local checks and
   CI report are attached to its pull request.
2. Restore cross-platform build and release evidence before claiming a usable
   release on Windows, Linux, or macOS.
3. Qualify force accuracy and invariants for TreePM and FMM before further
   performance tuning or scientific claims.
4. Qualify the comoving cosmology mode against its analytic and spectral
   references before presenting it as a cosmology solver.
5. Reduce the Qt editor modules under #440 while retaining background-only UI
   test behavior.
6. Resume feature work one issue per branch; do not accumulate another mixed
   worktree checkpoint.
