# Decision 060: Compensated Accumulation Policy

Status: accepted
Issue: #673
Plan version: 1.0.44

## Context

Force, energy, and diagnostic reductions have different numerical and
performance contracts. The Direct CPU solver is the reference path, while
conservation metrics are diagnostics and may trade vectorization for better
cancellation behavior. Selecting a compensation scheme without measuring the
same ordered terms would either change the force oracle silently or make a
performance claim without evidence.

## Decision

`src/physics/reduction/ScalarReduction.hpp` and its implementation provide
three explicit ordered policies: plain addition, Kahan compensation, and
Neumaier compensation. They allocate nothing and do not reorder terms.

The Direct CPU force path remains plain addition. This preserves its numerical
oracle, SIMD-compatible reduction shape, and existing singularity and finite
state behavior. Barnes-Hut and FMM force accumulations remain unchanged by this
issue as well.

`ConservationMetrics` uses Neumaier by default for kinetic energy, potential
energy, and momentum diagnostics. An internal policy overload keeps Plain and
Kahan available for qualification without widening the public SDK ABI. The
KDK state update still receives forces from the unchanged Direct CPU path; the
diagnostic policy cannot alter particle state.

Kahan remains a measured alternative but is not selected: the bounded
cancellation matrix compares its error and throughput against Plain and
Neumaier under the same order. The long-run case measures all three policies
over 4096 KDK steps and verifies that the default diagnostic call is exactly
the selected Neumaier call.

## Invariants

- The force reference is deterministic plain accumulation.
- Diagnostic terms use fixed `i < j` pair order and fixed particle order.
- Compensation never accepts non-finite input as a valid result.
- Invalid state and zero-softening singularity behavior remains transactional.
- The reduction harness uses exact IEEE-754 bit hashes for repeatability.
- Generated reduction evidence is written outside the source tree.

## Evidence

`TST-P1-006` evaluates the three policies over force, kinetic-energy,
potential-energy, and momentum cancellation workloads. It also runs the
existing Direct/KDK path for 4096 steps and records diagnostic drift. The
external runner validates the complete matrix, selected-policy mapping,
ordering hashes, finite results, cancellation error, and long-run limit against
`plan/reduction.json`.
