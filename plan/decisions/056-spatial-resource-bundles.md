# Decision 056: Spatial Resource Views and Bundles

Status: accepted
Issue: #681
Plan version: 1.0.40

## Context

The solver resource contract now distinguishes algorithms from the resources
they require, but the implementation still let Barnes-Hut and FMM own raw
optional tree instances directly. Traversal also consumed the mutable tree
owner, which made lifetime, replacement, and stale-resource validation
implicit.

## Decision

`Octree` remains the bounded storage owner for topology, geometry, stable
particle ordering, and cell ranges. `OctreeView` is the read-only execution
view. It exposes only spans and validated metadata; it carries the storage
generation and capacity contract, and a view is current only for the exact
resource that produced it.

`OctreeResource` owns one configured `Octree` and is the only component that
prepares it. Preparation first validates capacity, then attempts a refit and
falls back to a bounded rebuild. A successful or failed mutating tree
operation advances the generation so a previous view cannot be reused after
storage changes.

`SolverTreeResources` is the typed tree-resource bundle for hierarchical
solvers. It owns one local and one remote `OctreeResource`; the remote resource
is required by split-domain evaluation. The active simulation variant contains
typed bundles: Direct contains no tree resource, while Barnes-Hut and FMM each
contain their solver and tree-resource bundle. Bundle move operations rebind
the solver's non-owning reference to the moved resource bundle.

Algorithm-specific state remains outside `Octree::Cell`. FMM multipoles stay
in `FmmSolver`; future KIFMM workspace is covered by the existing resource
contract and is not materialized here. No Grid view or Grid bundle is created
until a production Grid root exists; PM and TreePM remain deferred.

## Invariants

- A Direct composition never constructs an `OctreeResource`.
- Hierarchical traversal accepts `OctreeView`, never a mutable `Octree` owner.
- A view must satisfy its generation, owner, particle-capacity, cell-capacity,
  and span-size checks before evaluation.
- Resource replacement is performed with a fully prepared candidate before the
  active local resource is replaced.
- Resource preparation and solver workspace capacity are established before
  the steady-state step; force evaluation performs no resource allocation.
- The public C and C++ SDK boundaries remain unchanged.

## Evidence

`TST-P3-001` and `TST-P3-002` preserve Barnes-Hut and FMM behavior. `TST-P3-003`
preserves the solver/resource matrix. `TST-P3-004` covers unprepared views,
generation invalidation, capacity rejection without mutation, move safety,
and typed local/remote resource ownership. The Release CPU build and the
allocation regression tests qualify the existing steady-state paths.
