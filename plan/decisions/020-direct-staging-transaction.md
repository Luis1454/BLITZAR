# Decision 020: Direct Solver Staging

Status: accepted
Plan version: 1.0.6

## Decision

`DirectSolver::ComputeRange` evaluates each source-target pair once. Each
target acceleration is written to a reusable per-solver staging vector only
after its pair validation and finite-result checks complete. A separate
target-sized publication loop updates the caller's force view after every
target has succeeded.

## Consequences

- Singularities, invalid pairs, and non-finite accumulations cannot partially
  publish forces.
- The previous validation pass and calculation pass no longer repeat all pair
  evaluations and particle reads.
- The staging vector grows only when a solver sees a larger target count and
  is reused by subsequent calls on that solver instance.
- A preallocated execution scratch state remains the follow-up boundary for the
  no-allocation hot-path work in issue #555.
