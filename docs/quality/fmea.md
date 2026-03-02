# FMEA Baseline

Scope: software-level failure analysis for deterministic simulation runtime and protocol layers.

| ID | Failure Mode | Effect | Detection | Mitigation | Residual Risk |
|---|---|---|---|---|---|
| FMEA-001 | Parser accepts malformed numeric input | Unstable runtime parameters | `ConfigArgsTest.TST_UNT_CONF_006_RejectsTrailingGarbageNumericArguments` | strict parsing with rejection paths | Low |
| FMEA-002 | Backend protocol status contract regression | Frontend incompatibility and silent faults | `BackendProtocolTest.TST_INT_PROT_001_BackendClientParsesStatusAndSnapshotFromRealBackend` | protocol compatibility tests + review gate | Medium |
| FMEA-003 | Unauthorized control command accepted | Unsafe remote control | `BackendProtocolTest.TST_INT_PROT_008_BackendRejectsRequestsWithoutConfiguredToken` | token enforcement and explicit unauthorized responses | Medium |
| FMEA-004 | Runtime fails to reconnect after backend restart | Command channel deadlock, UI stale state | `FrontendBridgeTest.TST_INT_RUNT_001_ReconnectsAfterRealBackendRestart` and `FrontendRuntimeTest.TST_CNT_RUNT_003_ReconnectsWhenRealBackendRestarts` | reconnect state machine with bounded retry loop | Medium |
| FMEA-005 | Energy drift exceeds accepted bound | Non-credible physics output | `PhysicsTest.TST_UNT_PHYS_002_EnergyConservation` and `PhysicsTest.TST_UNT_PHYS_006_EnergyConservationHighMassNoSph` | threshold-based validation and nightly campaign | High |
| FMEA-006 | Legacy build target reintroduced | Unsupported runtime topology in production | `TST_QLT_REPO_003_GravityNoLegacyCheck` | explicit policy check in CI | Low |
| FMEA-007 | Dynamic module behavior leaks into critical `prod` path | Loss of determinism and qualification ambiguity | architecture review + `TST_QLT_REPO_004_GravityRepoPolicyCheck` + standards profile review | keep mission-critical path static in `prod`, forbid runtime reload in critical path | Medium |

## Maintenance Rules

- Add one FMEA line for each new safety- or mission-impacting failure mode.
- Every FMEA line must point to at least one verification artifact.
- Keep mitigation and detection synchronized with `docs/quality/quality_manifest.json` traceability section.
