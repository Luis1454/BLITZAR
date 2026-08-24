# Decision 026: MPI Overlap Qualification

Status: accepted
Plan version: 1.0.10

## Decision

The distributed dispatcher keeps overlap as the production mode and exposes a
non-public serialized mode only for qualification. Both modes post the same
halo exchange and execute the same local and remote force contracts; the
serialized mode completes the halo before local force work so the useful
calculation/network overlap can be isolated without changing numerical input.

Each distributed force phase records a fixed-size `MpiOverlapTrace` containing
the event order, elapsed intervals, packet counts, and wire bytes observed by
the persistent `MpiGhostExchange`. The `TST-P8-003` test runs both modes on the
same deterministic workload and reports volume, total time, speedup, and state
parity. It accepts the timeline ordering and parity; it does not claim a
positive speedup from a single noisy host measurement.

The Direct HIP range path uses separate persistent target and source staging
buffers. It uploads only the target prefix and the requested source range,
while force downloads remain limited to the target count. Barnes-Hut retains a
complete tree transfer because its current aggregate contract is not additive
over source ranges.

## Consequences

- MPI overlap is demonstrated by an event timeline rather than inferred from
  source ordering alone.
- Transfer volume is reported from the wire sizes actually posted to MPI.
- CPU-only builds keep the same trace and serialized baseline without HIP
  headers or device requirements.
- GPU runtime qualification remains capability-gated; compile-only support is
  not reported as device execution evidence.
