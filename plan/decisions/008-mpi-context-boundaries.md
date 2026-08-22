# Decision 008: MPI Session and Transport Boundaries

Status: accepted
Plan version: 1.0.6

## Decision

`MpiContext` remains the stable internal facade used by the SDK, but it does
not own MPI policy directly. `MpiSession` is the sole owner of process-level
MPI initialization and finalization. It requests `MPI_THREAD_MULTIPLE`, tracks
whether BLITZAR initialized MPI, and never finalizes a session initialized by
the caller.

`MpiCollectives` owns checked status synchronization and scalar reductions.
`MpiPacketTransport` owns validated `MPI_Alltoall[v]` and `MPI_Allgather[v]`
operations. `MpiGhostTransport` owns non-blocking halo requests, while
`MpiGhostExchange` owns their move-only state and cancels active requests on
destruction when MPI is still live.

All four components keep MPI headers and handles in implementation files. The
facade preserves the existing span and packet contracts, so domain decomposition
and solver orchestration do not depend on MPI types or lifecycle details.

## Consequences

- Nested `MpiContext` instances share one session reference without duplicate
  initialization or premature finalization.
- An externally initialized MPI process remains owned by its caller after the
  last BLITZAR context is destroyed.
- Every non-blocking exchange has `Begin`, `Complete`, `Abort`, and destructor
  completion paths.
- Error logs identify the transport component, rank, phase, local status, and
  synchronized status.
- `tests/Mpi.cpp` validates external ownership, nested contexts, internal
  ownership, invalid collective layouts, and ghost abort/recovery paths.
