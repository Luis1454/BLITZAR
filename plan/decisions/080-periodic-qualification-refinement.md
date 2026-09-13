# Decision 080: Periodic qualification refinement

Status: accepted
Plan version: 1.0.64

## Context

Issue 741 (`P3-PBC-001`) shipped a qualification pair in
`tests/physics/periodic/`. An architecture review then exposed two avoidable
weaknesses: the fused image-region case in `PeriodicDomainTest.cpp` exceeded
both the function-length (81/80) and branching (21/12) caps, and the
per-solver parity block was duplicated verbatim across the four hierarchical
cases of `PeriodicSolverTest.cpp`.

## Decision

Split the image-region oracle into three responsibility-focused cases and
extract `Distance` and `SamePoint` helpers so every domain-test function stays
under the production thresholds and the previously accepted architecture
waiver is removed.

Introduce a one-parameter `EvaluateSolverCase` template (per-solver arguments
bundled in the `SolverCase` value struct) that owns solver construction and a
single periodic evaluation, and keep every comparison predicate in the caller.
The Direct/BH/FMM non-regression checks remain strict bit-exact (`== 0.0`
failure semantics preserved through `!= 0.0`); only KIFMM retains its bounded
`> 1.0e-12` tolerance because its surface algebra is not bit-identical to the
Direct oracle.

Promote the repeated box, count, opening, softening, and tolerance literals to
file-scope `constexpr` constants. The internal-include waiver (14 > 12) for
`PeriodicSolverTest.cpp` is retained because the test topology links every
hierarchical solver family against one Direct oracle.

## Constraints

- The helper is bounded to one aggregate parameter and two branch points, so
  `max_parameters` (4) and `max_branch_points` (12) retain headroom.
- Bit-exact `!= 0.0` comparison remains the regression oracle on identical
  code paths; it is never replaced by a floating tolerance.
