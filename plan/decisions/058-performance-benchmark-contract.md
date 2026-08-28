# Decision 058: Reproducible Performance Benchmark Contract

Status: accepted
Issue: #671
Plan version: 1.0.42

## Context

Decision 032 established reproducible scaling evidence, but its records did not
make per-step timing, steady-state allocation behavior, throughput, or the
particle distribution an explicit contract. Later optimization issues would
otherwise compare results whose workload and measurement boundaries were only
implicit.

## Decision

The existing `blitzar_scaling_test` and
`tools/evidence/release_evidence.py` remain the single benchmark path. The v2
contract in `plan/scaling.json` defines two deterministic distributions:
`box-pair-v1` for solver comparison and `boundary-crossing-v1` for migration.
The seed, distribution definition, warmup count, timed-step count, and Direct
CPU oracle tolerance are versioned inputs.

Each rank record contains wall time, mean/min/max step time, steady-state
allocation count, allocation-free status, peak resident memory, throughput,
backend, distribution, and oracle fields. The evidence runner rejects a record
with a stale schema, mismatched distribution, missing metric, non-positive
timing, or a post-warmup allocation. Throughput is derived from the global
particle count, timed steps, and critical-path elapsed time.

Environment metadata explicitly records compiler, CPU capability, GPU
capability, requested thread count, backend, and the problem-size field. Raw
logs and reports remain outside the source tree. A local run may be used for
development evidence; multi-rank single-host results are never presented as
multi-node evidence.

## Invariants

- Direct CPU is the numerical oracle for Direct, Barnes-Hut, and FMM CPU runs.
- Optimized solver records must pass the declared `max_abs_state_error`
  tolerance against the same deterministic input and step sequence.
- The record schema and distribution identifier must match the frozen contract.
- The timed region starts after warmup and must perform zero host allocations.
- Every benchmark result identifies its problem size and measurement units.
- No solver, layout, public ABI, or simulation algorithm changes are part of
  this issue.

## Evidence

`TST-P1-004` exercises a hierarchical solver with the Direct CPU oracle.
`CHK-P0-028` validates the versioned contract and `CHK-P0-029` validates its
parser and fixture behavior. The release evidence runner produces external
`metadata.json`, `results.json`, `summary.md`, and raw logs for the complete
strong, weak, migration, and overlap matrix.
