# Test Expansion Plan for BLITZAR Coverage Improvement

**Status**: Draft  
**Date**: 2026-03-27  
**Target Coverage**: Lines 47.6% → 65%, Functions 45.4% → 60%, Branches 32.8% → 50%

---

## 1. Current Coverage Analysis

### Metrics (from coverage-data branch)
| Metric | Current | Target | Gap |
|--------|---------|--------|-----|
| Lines | 47.6% | 65% | +17.4% |
| Functions | 45.4% | 60% | +14.6% |
| Branches | 32.8% | 50% | +17.2% |

### Test Distribution
| Area | Files | Tests | Ratio |
|------|-------|-------|-------|
| physics | 12 | (in tests/unit/physics) | ✅ Good |
| protocol | 4 | (in tests/unit/protocol) | ⚠️ Low |
| client | 2 | (in tests/unit/module_client) | ⚠️ Very Low |
| config | 6 | (in tests/unit/config) | ✅ Good |
| **Priority Areas** | | | |
| engine/src/server | 2 cpp files | ~0 dedicated tests | ❌ Critical Gap |
| runtime/src/protocol | 7 cpp files | ~1 test file | ❌ Critical Gap |
| runtime/src/client | 11 cpp files | ~2 test files | ❌ Critical Gap |

---

## 2. Priority Expansion Areas (in order)

### Phase 1: Protocol Stack (P0)
**Target**: +8% line coverage, +12% branch coverage  
**Scope**: runtime/src/protocol/*.cpp

Files to test:
- `ServerJsonCodec.cpp` (core serialization)
- `ServerJsonCodecParse.cpp` (JSON parsing)
- `ServerJsonCodecParseStatus.cpp` (status messages)
- `ServerJsonCodecParseSnapshot.cpp` (snapshot parsing)
- `ServerJsonCodecReadNumber.cpp` (number parsing)
- `ServerProtocol.cpp` (protocol state machine)
- `ServerClient.cpp` (client connection handling)

Current test coverage: `tests/unit/protocol/json_codec.cpp` (~28 handlers)

**Gaps to fill**:
- ❌ Edge cases in number parsing (precision, overflow)
- ❌ Malformed JSON error paths
- ❌ Status message parsing variations
- ❌ Snapshot data integrity validation
- ❌ Connection state transitions
- ❌ Protocol versioning compatibility

**Estimated tests needed**: 40-60 new test cases

---

### Phase 2: Client Runtime (P1)
**Target**: +5% line coverage  
**Scope**: runtime/src/client/*.cpp

Files to test:
- `ClientModuleApi.cpp` (API entry points)
- `ClientModuleBoundary.cpp` (boundary interface)
- `ClientServerBridge.cpp` (bridge protocol)
- `ClientRuntime.cpp` (runtime lifecycle)
- `ErrorBuffer.cpp` (error handling)

Current test coverage: Very sparse

**Gaps to fill**:
- ❌ Module loading/unloading lifecycle
- ❌ Error propagation paths
- ❌ Bridge command sequencing
- ❌ Resource cleanup on failure

**Estimated tests needed**: 25-35 new test cases

---

### Phase 3: Server Lifecycle (P2)
**Target**: +4% line coverage  
**Scope**: engine/src/server/*.cpp

Files to test:
- `SimulationServer.cpp` (server state machine) - 3501 lines, very complex
- `SimulationInitConfig.cpp` (configuration handling)

Current test coverage: Integrated tests only, no unit tests

**Gaps to fill**:
- ❌ Configuration parsing edge cases
- ❌ Server initialization sequences
- ❌ Error recovery paths
- ❌ Resource allocation/deallocation

**Note**: SimulationServer is too large (3501 lines) for unit tests. Defer to Lot C refactoring.  
**Priority**: SimulationInitConfig only (~200 lines, testable)

**Estimated tests needed**: 15-25 new test cases

---

## 3. Implementation Strategy

### Batch 1: Protocol Codec Edge Cases (Quick Win)
```
Issue: #329 "Expand protocol codec test coverage"
Branch: issue/329-protocol-codec-tests
Scope: runtime/src/protocol (codec + parsing)
Tests: 40-50 new cases
Est. Coverage gain: +5-7% lines, +8-10% branches
Timeline: 1 cycle
```

**Test categories**:
1. Number parsing (float32 precision, NaN, Inf, denormalized)
2. JSON malformation handling
3. Status message state machine
4. Snapshot binary format validation
5. Connection lifecycle

### Batch 2: Client Module Boundaries (Medium)
```
Issue: #330 "Improve client module test coverage"
Branch: issue/330-client-module-tests
Scope: runtime/src/client (API + lifecycle)
Tests: 25-35 new cases
Est. Coverage gain: +4-5% lines, +6-8% branches
Timeline: 2 cycles
```

### Batch 3: Server Configuration (Supporting)
```
Issue: #331 "Add SimulationInitConfig unit tests"
Branch: issue/331-server-config-tests
Scope: engine/src/server (config only)
Tests: 15-25 new cases
Est. Coverage gain: +2-3% lines
Timeline: 1 cycle
```

---

## 4. Incremental Targets

| Cycle | Batch | Target Lines | Target Functions | Target Branches | PR |
|-------|-------|--------------|------------------|-----------------|-----|
| Current | Baseline | 47.6% | 45.4% | 32.8% | #325 ✅ |
| +1 | Protocol | 52-54% | 50-52% | 40-42% | #329 |
| +2 | Client | 56-58% | 54-56% | 46-48% | #330 |
| +3 | Config | 59-61% | 57-59% | 49-51% | #331 |
| +4+ | Architectural | 65%+ | 60%+ | 50%+ | Future |

---

## 5. Quality Gates for New Tests

Every test batch must satisfy:
- ✅ All tests pass deterministically
- ✅ No environment-dependent behavior
- ✅ Coverage metrics increase by stated amount
- ✅ Nightly workflow generates updated coverage report
- ✅ Regression suite protected by tests (no fallback)
- ✅ Anti-recurrence tests for newly covered paths

---

## 6. Next Step

**Action**: Open issue #329 with Batch 1 scope (Protocol Codec)

```
Title: Expand protocol codec test coverage (+5% target)
Labels: quality, testing
Scope: runtime/src/protocol
Target coverage gain: +5-7% lines, +8-10% branches
Tests: 40-50 new cases covering number precision, JSON malformation, status parsing
Timeline: 1 cycle
```

---

**Owner**: AI Agent  
**Status**: Ready for execution  
**Decision Required**: Approve Batch 1 scope to open issue #329?
