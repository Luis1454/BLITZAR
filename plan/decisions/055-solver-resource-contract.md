# Decision 055: Orthogonal Solver Resource Contract

Status: accepted
Issue: #680
Plan version: 1.0.39

## Context

The solver variant currently identifies Direct, Barnes-Hut, and FMM, while
the dispatch layer also carries backend state, tree settings, and several
solver-specific request shapes. This makes it possible to add an algorithm
without making its resource ownership explicit. KIFMM, PM, and TreePM need a
stable resource contract before their implementations are introduced.

The current implementation observations are deliberately recorded rather
than silently changed here:

- `SimSolverVariant` contains Direct, Barnes-Hut, and FMM only.
- Barnes-Hut and FMM currently own their lazy Octree instances internally.
- `FmmSolver` reuses Barnes-Hut settings and split-request aliases.
- `SimSolverDispatch` exposes solver-specific overloads and backend branches.
- PM and TreePM are rejected as unsupported before solver construction.

## Decision

`src/solvers/SolverContract.hpp` is the single internal identity and resource
requirement contract. `SolverKind` retains the existing values `0` through
`4`; `Kifmm = 5` is an internal planning identity only. No public C ABI or
public C++ enum is changed by this decision.

`SolverResourceContract::For` maps each algorithm to exactly one validated
resource shape:

| Algorithm | Spatial resource | Private workspace |
| --- | --- | --- |
| Direct | none | none |
| Barnes-Hut | Octree | none |
| FMM | Octree | none |
| KIFMM | Octree | kernel-independent multipole |
| PM | Grid | none |
| TreePM | Octree and Grid | none |

The descriptor is not an owner and does not allocate. Issue #681 materializes
the matching typed ownership bundles and read-only resource views. Until
then, the descriptor prevents a solver contract from silently claiming a
different resource shape. `Sim` keeps spatial resources optional at its
composition boundary: Direct must not create an Octree, and a hierarchical
solver must fail preparation if its required resource is unavailable.

Execution backend and time integration remain orthogonal axes. The resource
contract does not include HIP, CUDA, MPI, or KDK types. The integration layer
will consume one force-evaluation contract in issue #682 rather than learning
about resource families or solver-specific split calls.

Preparation validates algorithm, resource shape, capacity, and capability
before particle mutation. Evaluation writes finite forces to staging storage.
Commit publishes the staged result transactionally; unsupported or invalid
capabilities return before mutation. These ownership and transactional
guarantees are implemented and qualified by issues #681 and #682, not
duplicated in this descriptor.

## Ownership Diagram

The target composition after #681 is:

```text
Sim
`-- optional spatial-resource coordinator
    `-- SolverBundle variant
        |-- Direct: no spatial resource
        |-- Barnes-Hut: Octree view + owned tree execution state
        |-- FMM: Octree view + owned multipole execution state
        |-- KIFMM: Octree view + private KIFMM workspace
        |-- PM: Grid view
        `-- TreePM: Octree view + Grid view
```

The solver owns algorithm-specific workspace. Shared spatial topology is
owned by the resource coordinator and consumed through a read-only view; it
is never added to `Octree::Cell` as algorithm-specific state.

## Failure Matrix

| Condition | Required result | Mutation allowed |
| --- | --- | --- |
| Unknown internal solver identity | Invalid argument | No |
| Contract shape does not match identity | Internal error | No |
| Known but deferred solver capability | Unsupported | No |
| Required resource is absent | Unsupported | No |
| Capacity cannot satisfy preparation | Invalid argument or allocation failure | No |
| Non-finite staged force | Internal error | No |
| Commit failure | Original state restored | No partial commit |

## Consequences

- Adding KIFMM does not require a public enum value in this phase.
- PM and TreePM resource shapes are frozen without creating deferred roots.
- #681 owns concrete resource bundles; #682 owns the single force-provider
  boundary and dispatch replacement.
- The existing FMM aliases and dispatch overload family remain explicit debt
  and are not copied into the new contract.
- `TST-P3-003` proves every current and planned resource shape, including the
  invalid identity/shape combination and public enum-value preservation.
