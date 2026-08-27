# Decision 025: Distributed Input Ownership

Status: accepted
Issue: #553
Plan version: 1.0.9

## Context

The previous MPI initialization path required every rank to receive and fill a
full-capacity particle arena before retaining its local prefix. That made
resident memory scale with `P*N` and hid the cost of distributed ownership.

## Decision

Rank zero is the authority for the initial `ParticleStateView`. Non-root ranks
may provide an empty view. `MpiDomainDecomposition` derives the deterministic
Morton split from the root state and broadcasts only split metadata. A
temporary initialization exchange distributes `ParticlePacket` records, after
which each rank stores only its local packets in its fixed-capacity `ParticleArena`.

Halo packets are stored in a separate `ParticleSourceBuffer`; they are not appended to
the owned arena. Packet wire storage, gather storage, and the source SoA are
lazy and bounded by the observed exchange or configured staging capacity. The
Barnes-Hut primary tree is constructed with local rank capacity, while its
remote tree is created only when remote sources are present.

The legacy global-index commit helper and the transaction's duplicated source
count were removed. Transaction snapshots now describe only the owned local
prefix, which makes rollback independent of remote halo state.

## Consequences

- Non-root ranks no longer allocate or initialize a global particle SoA during
  `SetParticles`.
- `GetState` remains a global operation and may allocate its gather/seen buffers
  on first use; this is an explicit output boundary, not persistent ownership.
- Exact long-range force parity is preserved because the complete exchanged
  remote packet set is still evaluated.
- MPI tests report peak RSS with `N` and `P` so the ownership scaling can be
  inspected for two and four ranks.
- A rank grows its local arena and solver staging only to the observed migration
  result, bounded by the global particle count, before publishing the new state;
  cross-rank load rebalancing remains a later issue.
