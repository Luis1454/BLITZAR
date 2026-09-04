# Decision 069: CPU KIFMM Workspace and Qualification Boundary

Status: accepted
Plan version: 1.0.53

## Decision

The CPU KIFMM implementation is an internal solver extension below the
existing `src/solvers/fmm` responsibility. It consumes only a validated,
read-only `OctreeView` and the typed tree-force request. It does not mutate
Octree cells, add KIFMM state to `Octree::Cell`, or change the public C ABI,
the public solver enumeration, CLI configuration, GPU dispatch, or MPI
composition.

KIFMM uses a fixed order of four unique tensor-product boundary nodes on an
inner equivalent surface and an outer check surface. For each occupied tree
cell, the workspace samples the softened Laplace kernel at the check surface,
applies a precomputed depth-specific inverse operator, and stores bounded
equivalent weights. A deterministic interaction traversal applies the shared
full-width opening criterion: leaf interactions use the Direct CPU pair law;
accepted internal cells use the equivalent weights and the kernel evaluated at
the target. The zero opening angle is the exact Direct-equivalent mode.

The `KifmmWorkspace` owns all equivalent nodes, check nodes, depth operators,
cell weights, interaction lists, traversal storage, and force staging. Its
capacity is fixed during construction or `Prepare`; force evaluation only
writes pre-sized spans. Operator construction uses deterministic pivoted
elimination and rejects non-finite or rank-deficient systems before force
commit. Failed evaluations leave the caller's force view unchanged.

`TST-P3-006` is the implementation boundary. It covers exact zero-angle
parity, Direct-reference approximation error, deterministic repeated runs,
finite empty/degenerate handling, explicit capacity rejection, singularity
rollback, shared-tree generation preservation, and zero steady-state
allocations. A separate qualification issue must promote KIFMM from deferred
to a reported capability and may add public selection only after this test and
the independent accuracy/rollback suite pass.

## Consequences

- KIFMM has explicit private workspace ownership without a generic solver or
  utility directory.
- The direct CPU solver remains the numerical reference for near-field and
  acceptance tests.
- The existing FMM, Barnes-Hut, PM, TreePM, MPI, and public API contracts remain
  behaviorally unchanged.
- KIFMM approximation evidence is local CPU evidence only; GPU, MPI, and
  multi-node claims remain deferred.
