# Critical Interface Contracts

This artifact defines the versioned contracts that gate server, protocol, and runtime-critical compatibility reviews.

## Contract Set

| Contract ID | Surface | Version | Canonical artifacts | Compatibility rule | Verification anchors |
|---|---|---|---|---|---|
| `CTR-PROT-001` | TCP JSON server protocol | `server-json-v1` | `docs/server_protocol.md`, `runtime/include/protocol/ServerProtocol.hpp` | additive optional fields are compatible; changed command names, mandatory fields, auth behavior, or snapshot/status semantics are breaking | `TST_INT_PROT_001..009` |
| `CTR-RUN-001` | client runtime control/query surface | `client-runtime-v1` | `runtime/include/client/IClientRuntime.hpp`, `runtime/include/client/ILocalServer.hpp` | method signature, lifecycle, and state semantics are stable within `v1`; any rename/removal/required call-order change is breaking | `TST_INT_RUNT_001..004` |
| `CTR-MOD-001` | client module ABI | `client-module-api-v1` | `runtime/include/client/ClientModuleApi.hpp` | loader compatibility requires unchanged entrypoint shape and `apiVersion`; ABI shape changes require a version bump | `TST_UNT_MODCLI_001..005`, `TST_UNT_MODHOST_001..003` |

## Contract Governance

- Contract artifacts are part of the quality baseline and must stay synchronized with requirement mappings in `docs/quality/quality_manifest.json`.
- Breaking changes require updating this file, the affected evidence refs, and the linked tests in the same change.
- Non-breaking additive changes still require review notes and test coverage for the new behavior.

## Breaking Change Workflow

1. Update the relevant contract row and canonical artifact(s).
2. Bump the documented contract version if compatibility is intentionally broken.
3. Update linked requirement evidence in the quality manifest.
4. Update or add tests that prove both the new behavior and the expected incompatibility boundary.
5. Surface the change in release review artifacts (`release_index.md`, `evidence_pack.md`) before merge.
