# Decision 017: Product and Plan Version Identity

Status: accepted
Plan version: 1.0.6

## Decision

`plan/manifest.json` is the single source of truth for two deliberately
separate identifiers:

- `product_version` is the semantic version of the public API and installed
  package. It is currently `1.0.0`.
- `plan_version` identifies the frozen clean-room implementation plan. It is
  currently `1.0.6`.

CMake reads both values from the manifest. The generated package config
exports `BLITZAR_VERSION`, `BLITZAR_PRODUCT_VERSION`, and
`BLITZAR_PLAN_VERSION`. The C and C++ SDK facades expose the same pair, and
the examples exercise those accessors.

## Consequences

- A package artifact can be compared to the plan revision that qualified it.
- Product releases do not require pretending that the roadmap revision is an
  API release.
- `plan_check` rejects invalid versions, stale `PLAN.md` mirrors, independent
  CMake version literals, and incomplete installed package metadata.
- A product release changes `product_version`; a plan change changes
  `plan_version`; both changes remain traceable in the manifest.
