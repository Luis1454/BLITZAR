# Decision 084: Collision and merger scope (PHYS-001)

Status: accepted
Plan version: 1.0.68

## Context

Issue 740 (`PHYS-001`) decides whether collision handling is a product
requirement and, if retained, defines detection radius, close-encounter
policy, elastic and inelastic response, merger conservation, identity
handling, rollback, and compatibility with the full solver matrix, MPI, and
the P10 SPH families. Plummer softening is explicitly a non-goal for contact.

## Decision

Retain one collision model, `sticky-mergers-v1`: a deterministic,
perfectly-inelastic sticky merger triggered when a pair reaches the physical
contact radius `R_contact` (an independent per-run parameter, never the
softening length) with a radial-approach condition, evaluated over the
deterministic local-neighbor graph. The merger conserves mass and momentum
exactly, places the survivor at the lower stable ID, resolves simultaneous and
transitive contact sets in ascending ID order, and records the kinetic deficit
as an explicit dissipated-energy diagnostic. Cross-ownership pairs transfer to
the survivor rank before merging; published state contains only the active set.

The model is decomposed into two implementation issues: `PHYS-002`
(implement the deterministic detection-and-response stage with rollback) and
`PHYS-003` (qualify conservation, identity, ownership, and determinism
invariants).

Explicitly rejected: elastic restitution (no clean-room model or evidence),
zero-distance softening as contact (confirmed non-goal), mass-loss and
radiation channels, subgrid fragmentation, and gas-dynamic mergers (deferred
to the P10 SPH contract).

## Constraints

- The detection stage runs between integrator stages of the existing fixed
  split with full checkpoint rollback; no new solver entry point and no
  force-path change.
- Gravity-only and P10 hydrodynamics paths are unchanged by this decision.
- This decision introduces no production code and no test id; the
  implementation issues register the tests.
- The contract is frozen in `plan/collision.json`.
