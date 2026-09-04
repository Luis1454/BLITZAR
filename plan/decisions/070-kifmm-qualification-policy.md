# Decision 070: CPU KIFMM Qualification and Public Promotion

Status: accepted
Plan version: 1.0.54

## Decision

Issue 684 is the independent qualification boundary for the CPU KIFMM
implementation introduced by Decision 069. `TST-P3-007` is a separate runtime
test from the implementation test and compares the same deterministic particle
state with the Direct CPU solver as the numerical oracle and with the existing
CPU FMM contract. Its acceptance cases cover force error in exact and
approximate modes, finite conservation metrics, KDK energy and momentum
conservation, stable Octree ordering, repeated-run byte parity, steady-state
allocation freedom, empty and degenerate inputs, explicit capacity exhaustion,
singularity handling, and a failed second evaluation.

The failed second evaluation is transactional: after one successful force
evaluation, a rejected follow-up must leave particle state, caller force
storage, and the active Octree generation unchanged. Capacity rejection is
checked before any resource replacement. The test uses only the single-rank
CPU provider and records no MPI, HIP, GPU FMM, or multi-node evidence.

KIFMM may be promoted from deferred to an implemented-qualified CPU capability,
and the public C/C++ solver selection plus `kifmm` configuration spelling may
be enabled, only after `TST-P3-007` and the existing P3/P6 focused checks pass.
The public capability report advertises the CPU solver identity only; compiled
backend presence and deferred GPU/MPI feature claims remain independent.

## Consequences

- KIFMM qualification is reproducible and independent of its implementation
  smoke test.
- Direct remains the numerical reference, while FMM remains the existing
  hierarchical comparison contract.
- Public promotion does not expand the MPI, HIP, GPU FMM, or multi-node
  qualification scope.
- The public solver value is appended to the stable C ABI enumeration and the
  C++ facade mirrors it without changing existing values.
