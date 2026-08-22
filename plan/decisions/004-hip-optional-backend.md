# Decision 004: Optional HIP Backend

Status: accepted
Plan version: 1.0.3

## Decision

HIP is an optional P4 backend detected by CMake in `AUTO` mode. The CPU build
must remain complete when ROCm, HIP headers, `hipcc`, or a GPU are absent. HIP
runtime types are confined to `.hip` implementation files and are not exposed
through the public C or C++ SDK headers.

The runtime owns pinned host staging, device allocations, and a non-blocking
stream. GPU force evaluation is transactional: device output is copied into
the host arena only after the kernel reports a successful finite result. A
missing device, initialization error, unsupported configuration, or runtime
failure causes the simulation to use its selected OpenMP CPU solver.

## Consequences

- `BLITZAR_HIP_MODE=AUTO` keeps ordinary CPU configuration portable.
- `BLITZAR_HIP_MODE=ON` is an explicit requirement and fails configuration if
  HIP cannot be configured.
- GPU numerical qualification is conditional on a visible HIP device; the same
  CTest executable must still qualify the fallback path without one.
- HIP kernel files use the `.hip` suffix and are covered by the repository
  naming gate.
