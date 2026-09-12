# Decision 072: Execution contract hardening

Status: accepted
Issue: #685 follow-up
Plan version: 1.0.56

## Context

The initial reproducibility contract recorded FMA and reduction policies but did
not make the selected policy observable in the CPU reference arithmetic. The
default dispatcher could also select HIP during a strict run, and the output
manifest described a fast run as `host-cpu` before execution had selected a
backend. The V1 snapshot payload contains particle state only, while the
contract described a complete restart state.

## Decision

Execution settings contain arithmetic and backend policy only. The generation
seed remains in the run configuration and simulation generation state; it is
not duplicated in the execution-policy value.

Strict mode is a CPU-reference mode. The dispatcher does not attempt HIP in
strict mode. The Direct CPU force path and KDK state updates consume the CPU
FMA policy; Direct force accumulation also consumes the ordered versus
compensated reduction policy without allocating in the hot path. Fast mode may
attempt the capability-gated HIP path and continues to disclose that its
reduction order is backend-defined.

Manifest execution metadata records a `backend` boundary. Strict runs record
`cpu` and `host-cpu`; fast manifests prepared before the first step record
`runtime-selected` for both backend and device. A fast run never advertises
bitwise reproducibility. The compensator field is mode-dependent
(`direct-plain` for strict and `backend-defined` for fast). The metadata
manifest schema is incremented to 2 so consumers cannot silently interpret
the old execution block.

Snapshot boundary readiness is expressed in terms of all nonblocking MPI
communication currently owned by `MpiContext`; the present implementation is
the persistent ghost exchange. V1 snapshots continue to serialize particle
state only. Integrator, RNG, compensator, domain, ownership, and rank-count
change semantics remain explicit OPS-002 work and are not inferred from the
V1 payload.

## Acceptance

- `TST-P6-015` rejects seed duplication in execution settings, verifies strict
  CPU and fast accelerator selection, checks policy-consuming arithmetic, and
  round-trips the version-2 manifest backend field.
- MPI exchange coverage proves that an active ghost exchange is not a snapshot
  boundary and that an unprepared or completed exchange is not active.
- `CHK-P0-048` and `CHK-P0-049` validate the revised reproducibility contract
  and its fixtures.
- The existing distributed restart acceptance remains same-rank-count only
  until OPS-002 defines and tests portable domain metadata.
