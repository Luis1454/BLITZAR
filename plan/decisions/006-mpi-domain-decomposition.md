# Decision 006: Optional MPI Domain Decomposition

Status: accepted
Plan version: 1.0.6

## Decision

MPI is an optional internal backend. `BLITZAR_MPI_MODE=OFF` or an unavailable
MPI implementation keeps the single-rank execution path unchanged. When MPI
is enabled, `MpiContext` owns process-level initialization only when the
application has not already initialized MPI, requests `MPI_THREAD_MULTIPLE`,
and exposes no MPI type through the public SDK.

The domain is partitioned by deterministic Morton-key slices. Particle IDs are
stable global input indices, while each rank stores its current local particles
in the prefix of a fixed-capacity SoA arena. Each completed KDK step is followed by
`MPI_Alltoallv` migration. Every force evaluation exchanges all remote packets
with non-blocking point-to-point operations. This is deliberately an exact
long-range gravity halo: exchanging only particles near a boundary would not
preserve Newtonian gravity or the direct-solver oracle.

## Consequences

- MPI is a composition adapter and does not alter the C ABI or C++ facade.
- CPU and GPU solvers receive a target count plus a source count through the
  internal `ParticleStateView`; only local targets are integrated and written.
- `GetState` uses `MPI_Allgatherv` and restores global order by particle ID.
- `TST-P7-001` and `TST-P7-002` qualify two- and four-rank parity at `1e-5`.
- The exact packet exchange is correct but not yet a communication-optimal
  neighbor-only halo; remote multipole exchange is a later optimization.
