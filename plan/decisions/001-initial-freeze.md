# Decision 001: Initial Clean-Room Freeze

Status: accepted  
Plan version: 1.0.0

## Decision

Start the rewrite from a small public SDK, a contract-oriented simulation
core, and a direct CPU numerical reference. Add spatial solvers, CUDA, grids,
and persistence only in the dependency order defined by `PLAN.md`.

The CLI lives outside `src` and consumes the SDK. The core does not contain a
server, RPC dispatcher, GUI, plugin loader, or distributed scheduler. Those
features can return only as separately specified adapters.

## Consequences

- The old repository is not a dependency, migration source, or test oracle.
- Numerical parity is measured against the direct CPU implementation.
- Ownership is split by lifetime and responsibility instead of accumulating in
  aggregate state structs.
- CUDA remains optional until the CPU contracts and references are qualified.
- Changes to this decision require a new plan version and an explicit decision
  record.
