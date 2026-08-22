# Decision 012: Installed Package Dependencies

Status: accepted
Plan version: 1.0.6

## Decision

`BLITZARConfig.cmake` loads `CMakeFindDependencyMacro` before importing
`BLITZARTargets.cmake`. It calls `find_dependency` only for dependencies that
were part of the installed BLITZAR target: OpenMP, MPI CXX, CUDA Toolkit for
CUDA-language backends, and the HIP package selected during configuration.

The package records the selected HIP package name (`hip` or `HIP`) in the
generated config so an installed export does not guess which provider to
load. CPU-only builds do not request MPI, HIP, or CUDA dependencies.

`tests/PackageConsumer.cmake` installs the configured build into a temporary
prefix and configures and builds an external CMake consumer against
`BLITZAR::blitzar`.

## Consequences

- Build-tree and install-prefix consumers resolve the same imported targets.
- Optional GPU and MPI packages remain optional for CPU-only installations.
- A package consumer test exercises configuration and transitive link setup,
  not merely header visibility.
