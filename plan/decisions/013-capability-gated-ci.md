# Decision 013: Capability-Gated CI Lanes

Status: accepted
Plan version: 1.0.6

## Decision

The workflow separates CPU-only, install-prefix package, MPI, native CUDA,
HIP, sanitizer, and clang-tidy lanes. Every backend lane records its compiler,
backend, device visibility, rank count, and skip reason in the GitHub step
summary.

MPI qualification installs Open MPI and executes the two-rank and four-rank
CTest cases under a hard timeout. The hosted lane is explicitly single-node;
it validates rank parity and failure propagation, not multi-node network
scaling.

CUDA and HIP lanes probe their toolchain before configuring. Missing compilers
produce a successful, explicit skip. A compiler without a visible device runs
compile-only and records that hardware qualification was skipped. Device
tests run only when the matching device probe succeeds.

The package lane builds a CPU-only installation and uses
`tests/package/PackageConsumer.cmake` to configure and build an external consumer from
a fresh install prefix.

## Consequences

- A green compile-only GPU job cannot be misread as GPU runtime evidence.
- MPI hangs fail the workflow instead of consuming an unbounded runner.
- CPU and package behavior remain continuously testable on hosted runners
  without optional accelerator hardware.
