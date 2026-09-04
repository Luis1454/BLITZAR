# Decision 071: Reproducibility and restart-state policy

Status: accepted
Issue: #685
Plan version: 1.0.55

## Context

Restart compatibility previously described the physical configuration and
snapshot representation, but it did not identify the arithmetic policy or the
toolchain state that makes a byte comparison meaningful. The CLI also had no
explicit distinction between a strict reproducibility run and a performance-
oriented run.

## Decision

The internal execution contract has two modes. `strict` uses disabled
deliberate FMA and ordered reductions for CPU, HIP, and MPI policy slots. It
advertises bitwise reproducibility only when the backend, compiler identity,
device identity, precision, plan version, and recorded state policy match.
`fast` records hardware FMA and backend-defined reductions and always reports
`bitwise_reproducible: false`; it is compared across backends by finite-state
and force tolerances only.

The optional configuration directive is
`execution(mode=strict|fast)`, with strict as the default. The run manifest
records the execution mode, per-backend FMA and reduction policies, precision,
compiler, device boundary, RNG policy, compensator policy, ordering policy,
and bitwise disclosure. Generation seed remains the RNG state identity.

Snapshot publication is a boundary operation: all in-flight MPI ghost
exchanges must be complete before state capture. The CLI checks the simulation
boundary before capturing a snapshot or diagnostic state. OPS-002 owns the
backend-neutral serialized state schema; this decision establishes the
compatibility policy that schema must carry.

The Direct CPU solver remains the numerical reference. This change does not
alter the public C ABI or claim that a runtime mode can override compiler-wide
floating-point flags; it makes the selected policy explicit and rejects
incompatible restart metadata.

## Acceptance

- `TST-P6-009` proves strict full-run versus save/reload/resume final snapshot
  byte identity through the production CLI.
- `TST-P6-015` proves mode validation, manifest round-trip, environment/state
  fields, and snapshot-boundary readiness.
- `CHK-P0-048` and `CHK-P0-049` validate the frozen contract and its fixtures.
