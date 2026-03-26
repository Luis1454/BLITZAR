# Simulation Backlog

Objective: Centralize remaining modifications and track progress.

Status note:
- This file is a local working scratchpad, not the authoritative issue tracker.
- Any `P0` or `P1` item listed here must also exist as a GitHub issue or a quality-baseline action before it can be considered actively tracked.
- The canonical remediation response to the March 2026 static audit is documented in `docs/quality/static_audit_remediation_plan.md`.

## Legend
- Priority: `P0` critical, `P1` important, `P2` nice-to-have
- Status: `[ ]` to do, `[~]` in progress, `[x]` done

## Architecture
- [ ] `P0` Stabilize the server/client separation (clear and versioned API).
- [ ] `P1` Add a remote control interface (socket/HTTP) to pilot the sim.
- [ ] `P1` Unify entry points (`headless`, `qt`) on the same server layer.
- [ ] `P2` Add a UI plugin mode (load an external client).

## Simulation
- [ ] `P0` Finalize the GPU octree (perf + exactness).
- [ ] `P0` Fix numerical stability in SPH mode (prevent explosion).
- [ ] `P1` Add physical validation scenarios (stable orbit, shock, cloud).
- [ ] `P1` Add automated metrics (energy drift, time/step, GPU occupancy).

## I/O and Formats
- [ ] `P0` Formally document the main snapshot format (`VTK`).
- [ ] `P1` Load a snapshot directly from the UI (drag/drop + button).
- [ ] `P1` Incremental export (autosave every N steps).
- [ ] `P2` Add metadata and format version in exports.

## UI / UX
- [ ] `P0` Ensure Qt visual refresh at a stable rate.
- [ ] `P1` Finalize multi-camera views (sync/unsync).
- [ ] `P1` 3-axis gimbal: precision, adjustable sensitivity, orientation reset.
- [ ] `P1` Energy graphs: zoom, pause, long history.
- [ ] `P2` Responsive UI (small screen layouts).

## Build / DevEx
- [ ] `P0` Make Qt build reliable on clean machine (deps + runtime plugins).
- [ ] `P1` Add CMake presets (`dev`, `release`, `profiling`).
- [ ] `P1` Single script for graphic deps installation.
- [ ] `P2` Clean up non-blocking compiler warnings.

## Tests
- [ ] `P0` Server regression tests (deterministic steps on fixed seed).
- [ ] `P1` Import/export round-trip tests (`vtk`, `xyz`).
- [ ] `P1` Minimal perf test (FPS budget depending on particle count).
- [ ] `P2` Local CI: build + client smoke test.

## Free Notes
- Add ideas, reproduced bugs, repro commands, screenshots here.

## Full Refactoring Plan
- [x] `P0` Reorganize the tree structure into domains (`apps/`, `engine/`, `modules/`) with consistent paths.
- [x] `P0` Reconnect the build to the new tree (`CMakeLists.txt` root + `tests/CMakeLists.txt`).
- [x] `P0` Revalidate full compilation (`make build-run` and `make build-ci`).
- [x] `P0` Stabilize Qt integration tests (`QtMainWindowIntegration.*` crash `0xc0000409` no longer reproduces on `main`; see issue `#301` and `docs/quality/static_audit_remediation_plan.md`).
- [ ] `P0` Split the monolithic CMake into modules (`cmake/targets/*.cmake`).
- [ ] `P0` Create domain libraries to remove source redundancy.
- [ ] `P1` Introduce a dedicated CUDA layer (object lib or specialized lib) to avoid unnecessary recompilations.
- [ ] `P1` Restrict includes per target (remove overly global include dirs).
- [ ] `P1` Unify test declaration (avoid double logic root/tests-integration).
- [ ] `P1` Clean up legacy directories that are now empty (`src/`, `include/`, `fragments/`) after final validation.
- [ ] `P1` Update the build/run docs with the new architecture (README + scripts + command examples).

Recommended Execution Order:
1. Validate `QtMainWindowIntegration.*` stays green on `main`.
2. Extract CMake into modules.
3. Introduce domain libraries.
4. Clean includes/targets and remove redundancy.
5. Purge empty legacy folders and finalize docs.

## History
- The complete conformity journal and refactoring batches are archived in `docs/history/modifs_journal_2026-02_03.md`.
