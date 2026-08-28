# Decision 057: Typed Force Evaluation Boundary

Status: accepted
Issue: #682
Plan version: 1.0.41

## Context

The previous solver dispatch layer combined solver identity, backend selection,
MPI overlap, and several solver-specific Compute overloads. Each new solver
family would have had to repeat those branches, while KDK remained coupled to
the dispatch request and its scratch state.

## Decision

`blitzar_solvers::SolverForceEvaluation` is the only force-evaluation contract
consumed by KDK. It contains borrowed target, source, and force views, the
execution settings, and the local or remote source kind. KDK does not name a
solver, backend, MPI type, or spatial resource.

`blitzar_solvers::SolverForceRequest` contains the typed requests owned by solver
boundaries:

- `SolverForceRequest::Direct` describes a direct range and self-interaction
  policy.
- `SolverForceRequest::Tree` describes an Octree view, its owning resource, source
  kind, accumulation policy, and self-interaction policy.

`SolverCpuForceProvider` maps the generic contract to the typed request through
`SolverCpuForceTraits`. `SimBackendForceProvider` adds optional accelerator
selection and CPU fallback. `SimDistributedForceProvider` owns the MPI overlap
and remote source phases while exposing the same generic contract.
`std::variant` remains at the `Sim` composition boundary and is visited once to
select the concrete provider instantiation.

## Invariants

- KDK calls only `Evaluate(SolverForceEvaluation)` through its generic provider
  parameter.
- Each callable has at most four parameters; state is grouped in typed request
  values or provider contexts.
- Direct, Barnes-Hut, and FMM preserve their existing CPU behavior and force
  staging semantics.
- Unsupported resource or backend paths return deterministically before
  mutating force or particle state.
- Remote distributed evaluation uses the CPU provider unless a future backend
  explicitly supports the remote source shape.
- HIP and MPI headers remain outside the public SDK and KDK interfaces.
- Adding a solver family extends traits and a typed request shape without
  adding solver-specific branches to KDK.

## Evidence

`TST-P3-005` covers the generic provider contract, direct local and remote
evaluation, and typed direct request behavior. The Release CPU build and full
CTest run qualify the existing Direct, Barnes-Hut, FMM, KDK, rollback, MPI
fixture, and HIP fallback paths. Capability-gated physical MPI and HIP
execution remain subject to their existing environment requirements.
