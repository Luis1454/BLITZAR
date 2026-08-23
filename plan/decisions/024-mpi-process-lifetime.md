# Decision 024: MPI Process Lifetime

Status: accepted
Issue: #545
Plan version: 1.0.8

## Context

An internally initialized MPI process cannot call `MPI_Init_thread` again after
`MPI_Finalize`. Finalizing MPI when the last temporary `MpiContext` is
destroyed made sequential SDK simulations invalid in the same process.

## Decision

`MpiSession` initializes MPI at the first internally owned context and
registers one process-exit finalizer. Destroying a session only releases its
reference; it never finalizes MPI while the process can still construct another
context. An externally initialized MPI process remains owned by its caller.
The constructor rejects an already finalized MPI runtime instead of calling
`MPI_Query_thread` after finalization.

## Consequences

- Sequential `MpiContext` and `Simulation` instances remain valid.
- Non-blocking exchange destructors can observe a live MPI runtime until exit.
- MPI is finalized exactly once when BLITZAR owns initialization, unless the
  caller finalizes it first.
- The public C ABI and C++ facade are unchanged.
