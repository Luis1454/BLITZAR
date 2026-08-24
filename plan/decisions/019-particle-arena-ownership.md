# Decision 019: Particle Arena Ownership

Status: accepted
Plan version: 1.0.6

## Decision

`blitzar_sdk::Simulation` owns one `ParticleArena` by value. The particle,
acceleration, and KDK checkpoint objects receive non-owning references to that
arena and never extend its lifetime.

The standalone size-based constructors used by focused unit tests retain
exclusive ownership through `std::unique_ptr`; they are independent buffers,
not aliases of a simulation arena. Borrowed constructors use
`std::reference_wrapper` and are invalidated on move from the view object.

## Consequences

- Destruction order is explicit: views are destroyed before the simulation
  arena.
- There is no reference-counting control block or ambiguous arena lifetime in
  the SDK composition root.
- Move, destruction, and failure behavior remains covered for both owned test
  buffers and borrowed simulation views.
- `plan_check` rejects reintroduction of `std::shared_ptr<ParticleArena>` in
  production or test code.
