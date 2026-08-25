# Decision 030: Build and Workspace Contract

Status: accepted
Plan version: 1.0.13

## Decision

The top-level `CMakeLists.txt` remains the composition root, while library,
package, executable, and test target ownership lives in dedicated files under
`cmake/`. The source-completeness gate scans every CMake composition file so
explicit source lists cannot silently omit a tracked implementation.

Build trees are external to the checkout. Local commands use
`../.blitzar-build`; CI uses `${RUNNER_TEMP}/blitzar-build`. The workspace gate
allows existing ignored legacy build directories for inventory purposes but
rejects any generated build or Python-cache artifact that becomes tracked.

The manifest-defined deterministic gates are run through
`tools/quality_gate.py`, which is the same command used locally and by the
workflow validation job. Build, package, MPI, and accelerator jobs remain
runtime qualification jobs and depend on that structural gate.

## Consequences

- CMake target ownership is visible without introducing generic source roots.
- Package consumers are tested from an install prefix rather than the source tree.
- CI no longer writes build products into the checkout.
- Workspace hygiene and structural qualification produce machine-readable
  evidence without treating ignored local artifacts as source changes.
