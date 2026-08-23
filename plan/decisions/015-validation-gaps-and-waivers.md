# Decision 015: Numerical and Capability Validation Gaps

Status: accepted
Plan version: 1.0.6

## Decision

The MPI test binary keeps one executable and exposes stable mode-specific
CTest IDs for the previously implicit contracts:

- `TST-P7-003` runs the synchronized failure and recovery path with one MPI
  rank;
- `TST-P7-004` runs Barnes-Hut migration with opening angle zero and compares
  the result to the Direct reference;
- `TST-P7-005` rejects a packet layout whose declared count cannot fit the
  available buffer before any collective transfers;
- `TST-P8-002` drives particles outside the fixed domain and verifies that a
  failed step restores the complete pre-step state.

The existing distributed rollback case covers a force failure after the
communication phase and retries against an independent reference simulation.
The GPU test prints an explicit capability-qualified skip when no device is
visible; the CI CUDA/HIP lanes decide whether hardware execution is run.

The following remain explicit waivers rather than silent claims:

- successful transfers near `INT_MAX` packets are not allocated by a unit test;
  chunk arithmetic and early rejection are tested, while such a fixture would
  require infeasible memory and transfer time;
- real multi-node MPI topology and RDMA behavior are not demonstrated by a
  single hosted runner; the CI lane is rank-parity on one node;
- physical GPU execution is unverified when the capability-gated runner has
  no compiler or device, while compile-only and CPU fallback behavior remain
  tested.

## Consequences

- Each feasible gap has a stable, deterministic test identifier.
- Unavailable hardware and cluster topology are visible waivers, not green
  runtime claims.
- The Direct single-rank CPU path remains the numerical oracle for the new
  Barnes-Hut distributed case.
