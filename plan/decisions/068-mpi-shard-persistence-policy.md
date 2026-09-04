# Decision 068: Explicit MPI and HIP Shard Persistence

- Status: active
- Scope: clean-room issue #685
- Plan version: 1.0.52

## Decision

Distributed CLI output uses explicit rank-local snapshot shards. A run with
`rank_count > 1` has one root manifest owned by rank zero and one state file per
rank for every completed output step:

```text
run/
  manifest.json
  states/state-00000000.rank-00000000.bin
  states/state-00000000.rank-00000001.bin
```

The rank component is eight decimal digits and is ordered by rank. The
manifest's completed-output entry contains the ordered `shards` array rather
than a single `path`. The root manifest records `rank_count` and uses
`rank_index: 0`; shard headers record the actual rank index.

Each shard carries a versioned `SnapshotFrameView` with
`distribution=Sharded`, `id_policy=GlobalStable`, and a `particle_count` equal
to its local payload count. Its IDs are strictly increasing, globally stable,
and are validated against the global particle count by the manifest consumers.
The single-rank header and file format remain unchanged.

The CLI captures the local simulation state through an internal, non-collective
state boundary. It never calls the distributed `GetState` gather for output.
All ranks prepare the same logical run, write only their own atomic shard, and
collectively publish the root manifest after every rank reports success. A
failed shard or manifest publication leaves no completed output and rejects a
rerun against the non-empty run directory.

Restart with a sharded source requires the same rank count. Rank zero reads and
validates every shard into the root initialization state; the existing bounded
MPI distribution then reconstructs rank-local simulation ownership. Post-process
reads all shards step by step, validates complete and unique global ID coverage,
and computes metrics only after explicit reconstruction. No path uses an
implicit full-state gather.

Binary and capability-gated HDF5 use the same shard layout and header contract.
HIP device execution uses the existing host-visible local-state transfer before
the same writers; no GPU or MPI type enters the snapshot or metadata modules.
Distributed online diagnostics remain unsupported until a separate global
reduction contract is versioned, and therefore fail before directory creation.
Multi-node and RDMA execution remain qualification work even when MPI is
available.

## Acceptance

`TST-P6-011` retains direct CPU versus single-rank MPI byte parity.
`TST-P7-006` launches the configured CLI with two ranks, checks deterministic
rank-shard names and root-manifest ordering, reads every shard header, verifies
that a rerun is rejected without mutation, and exercises the explicit
distributed restart path. `TST-P6-002`, `TST-P6-012`, and the capability-gated
HDF5 tests remain the binary/HDF5 codec oracles.
