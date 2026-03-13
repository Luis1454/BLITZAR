# FMEA Action Matrix

Scope: software-level failure analysis for deterministic simulation runtime and protocol layers.

| ID | Failure Mode | Residual Risk | Owner | Status | Linked Tasks | Closure Evidence |
|---|---|---|---|---|---|---|
| FMEA-001 | Parser accepts malformed numeric input | Low | `config-maintainer` | `closed` | n/a | `EVD_TEST_UNIT_CONFIG_ARGS_CLI` |
| FMEA-002 | Server protocol status contract regression | Medium | `protocol-maintainer` | `in-progress` | `#120`, `#101`, `#98` | `EVD_TEST_INT_PROTOCOL_CONNECT`, `EVD_QLT_INTERFACE_CONTRACTS` |
| FMEA-003 | Unauthorized control command accepted | Medium | `security-maintainer` | `closed` | `#101` | `EVD_TEST_INT_PROTOCOL_CONTROL` |
| FMEA-004 | Runtime fails to reconnect after server restart | Medium | `runtime-maintainer` | `in-progress` | `#120` | `EVD_TEST_INT_RUNTIME_BRIDGE`, `EVD_TEST_INT_RUNTIME_RUNTIME` |
| FMEA-005 | Energy drift exceeds accepted bound | High | `physics-maintainer` | `in-progress` | `#102` | `EVD_TEST_UNIT_PHYSICS_ORBIT`, `EVD_TEST_UNIT_PHYSICS_MULTIBODY`, `EVD_QLT_NUMERICAL_CAMPAIGN` |
| FMEA-006 | Legacy build target reintroduced | Low | `build-maintainer` | `closed` | n/a | `EVD_CHECK_NO_LEGACY` |
| FMEA-007 | Dynamic module behavior leaks into critical `prod` path | Medium | `architecture-maintainer` | `in-progress` | `#118`, `#101`, `#206` | `EVD_CHECK_MAIN`, `EVD_QLT_STANDARDS_PROFILE`, `EVD_QLT_PROD_BASELINE`, `EVD_QLT_INTERFACE_CONTRACTS` |

## Maintenance Rules

- Add one FMEA line for each new safety- or mission-impacting failure mode.
- Keep `docs/quality/manifest/fmea_actions.json` synchronized with this matrix.
- Every `closed` mitigation must reference at least one verification evidence ID.
- Every High/Medium residual risk must keep at least one linked task in the action register.
- Nightly lanes publish a risk-status snapshot from the canonical action register.
