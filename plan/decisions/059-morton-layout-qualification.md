# Decision 059: Morton Ordering and Particle Layout Qualification

Status: accepted
Issue: #672
Plan version: 1.0.43

## Context

The Octree and MPI domain partition already share the Morton key and the
key-then-original-index ordering rule. The particle production boundary is a
64-byte-aligned SoA owned by `ParticleArena`. Replacing that contract with an
AoSoA or a new ordering without measurements would change locality, ownership
ties, memory use, and the future grid boundary at the same time.

## Decision

Issue #672 is a qualification boundary, not a production representation
migration. `tests/layout` measures the existing stable comparison ordering and
a bounded eight-pass LSD radix candidate over 64-bit Morton keys. Both paths
must order by `(MortonKey, original_particle_index)` and produce the same exact
order hash.

The harness compares the selected SoA representation with AoSoA candidates of
tile widths 4, 8, 16, and 32. Candidates are test-only and must materialize to
the current SoA view before the real Octree is built. The Octree hash and the
logical state hash are therefore required to match across every candidate.
The SoA representation remains selected at both the Octree and future grid
boundaries until a later issue changes that contract with new evidence.

The harness reports sort, materialization, Octree build, and contiguous scan
timings, modeled 64-byte cache-line visits, candidate and adapter memory, and
locality. Cache-line visits and scan throughput are deterministic proxies, not
hardware performance-counter claims. Hilbert ordering, unbounded tile
configuration, and GPU-only layouts are explicitly excluded.

## Invariants

- The MPI ownership tie rule remains `(MortonKey, original particle index)`.
- Comparison and radix ordering are deterministic and byte-repeatable.
- Layout candidates cannot alter the production `ParticleStateView` contract.
- The reference representation is SoA; no AoSoA candidate is selected by this
  issue.
- Generated benchmark reports and logs remain outside the source tree.

## Evidence

`TST-P1-005` runs the complete 3-count by 2-ordering by 5-representation
matrix. `tools/evidence/layout_evidence.py` validates the external record set,
exact hashes, positive metrics, and all representation invariants against
`plan/layout.json`.
