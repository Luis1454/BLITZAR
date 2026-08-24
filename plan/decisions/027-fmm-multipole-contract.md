# Decision 027: Deterministic FMM Multipole Contract

Status: accepted
Plan version: 1.0.11

## Decision

P3 materializes `src/solvers/fmm` as a CPU-only hierarchical solver. The
solver reuses the bounded Morton/Octree ownership contract and evaluates a
fixed order-2 gravitational expansion for well-separated cells:

- the monopole is the cell mass at its center of mass;
- the centered second moment supplies the quadrupole correction;
- leaf and non-separated interactions are evaluated particle by particle;
- `opening_angle == 0` is the deterministic direct-equivalent reference mode.

FMM writes only to pre-sized staging buffers and commits forces after every
target has completed successfully. Local and remote MPI source packets use
the same split request, while HIP dispatch remains unsupported for FMM until
a later accelerator qualification.

## Consequences

- Direct remains the numerical oracle; FMM reference tests report force and
  conservation error instead of claiming bitwise equality for approximated
  cells.
- Refit is used when particle membership remains inside the existing leaves;
  a failed refit rebuilds the bounded Octree before multipoles are evaluated.
- The FMM root can be promoted in the manifest without creating a generic
  solver or utility directory.
