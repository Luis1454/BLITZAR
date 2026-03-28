# Critical Interface Contracts

This artifact defines the versioned contracts that gate server, protocol, and runtime-critical compatibility reviews.

## Contract Set

| Contract ID | Surface | Version | Canonical artifacts | Compatibility rule | Verification anchors |
|---|---|---|---|---|---|
| `CTR-PROT-001` | TCP JSON server protocol | `server-json-v1` | `docs/server-protocol.md`, `runtime/protocol/server-protocol-schema.json`, `runtime/include/protocol/ServerProtocol.hpp`, `rust/blitzar-protocol/src/lib.rs`, `rust/blitzar-protocol/src/schema.rs` | additive optional fields, additive optional request fields, and additive commands are compatible; changed command names, mandatory fields, auth behavior, or snapshot/status semantics are breaking | `TST_INT_PROT_001..009`, `TST_UNT_PROT_001..005`, `TST-UNT-RPROT-001..006` |
| `CTR-WEB-001` | browser transport adapter | `web-gateway-v1` | `docs/web-gateway.md`, `docs/server-protocol.md`, `rust/blitzar-web-gateway/src/lib.rs` | additive optional HTTP fields and additive WebSocket event payload fields are compatible when backend `server-json-v1` command names and payload semantics remain unchanged; renamed routes, renamed event types, or rewritten backend command semantics are breaking | `TST-UNT-RWEB-001..005` |
| `CTR-RUN-001` | client runtime control/query surface | `client-runtime-v1` | `runtime/include/client/IClientRuntime.hpp`, `runtime/include/ffi/BlitzarRuntimeBridgeApi.hpp`, `runtime/include/client/RustRuntimeBridgeState.hpp`, `rust/blitzar-runtime/src/lib.rs` | method signature, lifecycle, server-service connector semantics, bridge-state queue semantics, and snapshot-cap clamping are stable within `v1`; any rename/removal/required call-order change is breaking | `TST_INT_RUNT_001..004`, `TST-UNT-RRUNT-001..004` |
| `CTR-CORE-001` | compute-core C ABI | `core-ffi-v1` | `runtime/include/ffi/BlitzarCoreApi.hpp` | opaque-handle function signatures and POD layout are stable within `v1`; exposing C++/CUDA implementation types or changing required call-order is breaking | `TST_UNT_CORE_001..007` |
| `CTR-MOD-001` | client module ABI and load policy | `client-module-api-v1` | `runtime/include/client/ClientModuleApi.hpp`, `runtime/include/client/ClientModuleManifest.hpp`, `runtime/include/client/ClientModuleHash.hpp` | loader compatibility requires unchanged entrypoint shape, stable `apiVersion`, an allowlisted `module_id`, and a sidecar manifest whose product metadata and `sha256` match the module binary; live `reload` / `switch` are not part of the `prod` contract | `TST_UNT_MODCLI_001..005`, `TST_UNT_MODHOST_001..010` |

## Contract Governance

- Contract artifacts are part of the quality baseline and must stay synchronized with requirement mappings in `docs/quality/quality_manifest.json`.
- Breaking changes require updating this file, the affected evidence refs, and the linked tests in the same change.
- Non-breaking additive changes still require review notes and test coverage for the new behavior.

## Breaking Change Workflow

1. Update the relevant contract row and canonical artifact(s).
2. Bump the documented contract version if compatibility is intentionally broken.
3. Update linked requirement evidence in the quality manifest.
4. Update or add tests that prove both the new behavior and the expected incompatibility boundary.
5. Surface the change in release review artifacts (`release-index.md`, `evidence-pack.md`) before merge.



