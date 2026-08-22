# Decision 010: SDK Context Ownership and Call Concurrency

Status: accepted
Plan version: 1.0.6

## Decision

`blitzar_context` is a creation capability. `blitzar_simulation_create`
validates it and constructs an independent simulation runtime; the simulation
does not retain a pointer to the context. Destroying the context after a
successful simulation creation is therefore defined and safe. A context or a
simulation handle must not be destroyed while another operation uses that
same handle, because a raw opaque handle cannot protect its own lifetime from
concurrent destruction.

Operations on one live `blitzar_simulation` are non-reentrant. The C ABI uses
an atomic call guard: the first operation enters, and a concurrent operation
returns `BLITZAR_STATUS_INTERNAL_ERROR` without touching simulation state.
This applies to queries as well as mutations. Calls on distinct simulation
handles may proceed independently. The C++ wrapper keeps its last-operation
status atomic and preserves `valid()` as handle validity, so a rejected
concurrent call does not invalidate an otherwise live object.

Move construction, move assignment, and destruction of a C++ `Context` or
`Simulation` remain object-lifetime operations and must be externally
synchronized against calls on that object.

## Consequences

- Context lifetime is decoupled from simulation lifetime and is tested through
  the C API.
- Reentrant C API calls fail deterministically at the handle boundary instead
  of racing internal MPI, GPU, or particle state.
- The C++ facade exposes the same contract without a stale non-atomic status
  cache.
- No context or MPI header leaks into the public SDK contract.
