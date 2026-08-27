# Decision 037: Cross-Platform P4 Qualification

Status: accepted
Issue: #541
Plan version: 1.0.21

## Context

P4 has two implementation paths: native NVIDIA CUDA through CMake's CUDA
language and AMD HIP through the ROCm toolchain. The qualification executable
is shared, but the previous evidence probe referenced an obsolete target and
the repository had no Windows accelerator lane. A compiler-only check is not
runtime evidence, and a missing device must not turn a fallback test into a
false hardware claim.

## Decision

Use `blitzar_accelerator_test` as the single Direct/Barnes-Hut parity and
fallback contract. The release evidence probe creates its external log
directory before execution and resolves that target on both Windows and other
platforms.

The CI workflow keeps one implementation path with a matrix for Windows
native CUDA and AMD HIP. Each matrix entry independently probes its compiler,
configures the matching CMake platform, builds only the P4 qualification
target, and runs `TST-P4-001`. A missing compiler skips configuration; a
missing device still runs the test so the CPU fallback and injected runtime
failure contracts are verified. Device execution is reported only when the
platform-specific probe succeeds.

The GPU test runs dispatcher fallback and injected-error cases before the
device branch, compares Direct and Barnes-Hut forces against the CPU reference
at `1e-5`, and emits the measured maximum errors in the qualification log.
CPU-only builds continue to compile and run without HIP, ROCm, CUDA, or a GPU.

## Consequences

- HIP/CUDA implementation types remain below `src/gpu` and outside public SDK
  headers.
- Linux and Windows use one CTest target and one numerical contract instead of
  duplicated backend-specific tests.
- Native CUDA qualification is locally demonstrated on Windows/MSVC/NVCC;
  AMD HIP SDK and hosted Windows hardware remain capability-gated until their
  own toolchains and devices are exposed.
- A passed capability-gated CI job does not upgrade P4 from
  `capability-gated` to `implemented-qualified` without recorded device
  evidence.
