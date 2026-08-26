# Decision 033: Reproducible Evidence Gates

Status: accepted
Plan version: 1.0.16

## Decision

The scaling contract is executable through `tools/evidence/release_evidence.py`, and
its pure expansion and parsing rules are covered by
`tools/evidence/release_evidence_test.py`. The CI `release-evidence` job builds the
scaling and HIP qualification probes with MPI enabled, runs the versioned
matrix in strict mode, and uploads the generated directory as an external
artifact.

The internal migration trace is intentionally diagnostic rather than public:
`MpiExchange` records remote packet counts and `Simulation` exposes the latest
record to the test harness. The harness aggregates the trace over warmup and
timed steps so migration is not lost when a later step has no transfer.

## Consequences

- Weak scaling keeps `particles_per_rank` fixed for every rank count.
- Every executed workload has rank records, command logs, memory, communication,
  overlap, migration, and oracle fields.
- CPU fallback, HIP compilation, and physical GPU execution remain separate
  evidence states.
- Local multi-rank results cannot be labelled multi-node qualification.
- The scaling CLI process arguments are registered as a borrowed process-entry
  boundary, matching the existing MPI integration test.
