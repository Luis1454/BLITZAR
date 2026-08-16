# Generated Artifacts

BLITZAR does not version compiled binaries or build products.

## Source Repository

The repository source tree must not contain executable or toolchain artifacts:

- `.exe`, `.dll`, `.so`, `.dylib`
- `.lib`, `.a`, `.obj`, `.o`, `.pdb`
- `.cubin`, `.ptx`, `.fatbin`
- release archives such as `.zip` and `.whl`

The repository policy check rejects these extensions outside generated directories.

## Local Generated Directories

The following directories are disposable outputs and are ignored by Git:

- `build-*` and `cmake-build-*`: CMake/Ninja build trees
- `dist/`: local release bundles and GUI deployment trees
- `artifacts/`: numerical, profiling, and validation evidence
- `exports/` and `outputs/`: simulation results

`dist/releases/` and `dist/local-linux/` are useful local smoke-test outputs, but they
are not release authority. The GitHub release workflow is the only distribution source
of truth and rebuilds its bundles from the tagged source revision.

## Cleanup Rule

Keep at most the active build tree and the latest smoke-test bundle locally. Historical
GUI snapshots, audit bundles, duplicate build trees, and test-run directories must be
removed after their evidence has been recorded in CI artifacts.

## Runtime Source Boundaries

Runtime source files are not generated artifacts and must remain small enough to audit.
The public `Bridge` facade owns the client-facing API only. Remote lifecycle, retry,
autostart, snapshot, and status translation belong to the private `RemoteSession`
component; command serialization belongs to `Commands` and initial-state translation
belongs to `InitialState`. New transport or serialization behavior must be added to
those focused components rather than expanding the facade again.
