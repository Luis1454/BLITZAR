# Decision 007: MPI KDK Overlap and In-Step Migration

Status: accepted
Plan version: 1.0.6

## Decision

`MpiContext` is the sole public MPI boundary. Its internal composition owns the
MPI session, checked collectives, packet transport, and opaque lifetime of a
non-blocking ghost exchange. `MpiDomainDecomposition` and `MpiExchange` operate
only on spans, packet buffers, counts, and statuses; they do not include MPI
headers or branch on `BLITZAR_HAS_MPI`.

The distributed KDK dispatcher starts the halo count and packet exchange before
computing local-source forces. Direct force accumulation is split into a local
range and a remote range, with the second range accumulated into the first.
Barnes-Hut executes its local tree evaluation while the halo is pending and
recomputes the complete tree after reception because hierarchical aggregates
are not additive by source range in the current tree contract.

After Drift, a KDK transition hook migrates packets with `MPI_Alltoallv` before
the second force evaluation. The hook validates the received packets, updates
the local logical prefix and IDs, and reports a replaced state. The integrator
then recaptures its KDK checkpoint at the new ownership boundary so a second-force
failure cannot apply a partial kick to the new local state.

## Consequences

- MPI-specific code is confined to the internal `parallel` transport boundary;
  public SDK headers, `MpiDomainDecomposition`, and `MpiExchange` remain MPI-free.
- CPU-only builds use the same `MpiContext` contract without MPI linkage.
- Direct MPI execution overlaps network progress with useful local force work.
- Barnes-Hut correctness is preserved at the cost of a second full tree
  evaluation after halo completion.
- `TST-P8-001` validates a particle crossing rank ownership after Drift.
