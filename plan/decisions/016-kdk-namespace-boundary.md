# Decision 016: KDK Namespace Boundary

Status: accepted
Plan version: 1.0.6

## Decision

Keep the public KDK integrator in the single-level `blitzar_integration`
namespace. Place its implementation helpers in the separately named,
single-level `blitzar_integration_kdk` namespace instead of a generic nested
`detail` namespace.

The plan gate rejects generic `detail` namespaces and qualified
`::detail::` references in production, test, example, and public header code.

## Consequences

- Helper ownership is explicit without exposing a generic catch-all namespace.
- The integrator's public type boundary remains stable.
- Existing internal helper call sites use the named KDK boundary.
- Future helper namespaces must identify their responsibility and remain
  single-level.
