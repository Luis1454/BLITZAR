# Decision 014: Plan Gate Scope and Evidence

Status: accepted
Plan version: 1.0.6

## Decision

`tools/gates/plan_check.py` enforces only deterministic repository-contract rules:
materialized and deferred roots, safe path syntax, phase identifiers, frozen
manifest structure, forbidden references, source naming, and textual CTest
registration.

`plan/quality.json` declares `evidence_policy: registration-only`. A quality
manifest command proves only that a test is registered in CMake; it never
proves that the executable ran or passed. Runtime evidence remains the output
of an actual build/test or CI job and is reported separately.

Function length/count, branching complexity, allocation behavior, include
dependencies, and responsibility boundaries are reported as informational
architecture rules. They are not guessed by a regex gate. A future qualified
analyzer may promote one of them to an enforced rule with its own fixture
tests and acceptance contract.

`tools/gates/plan_check_test.py` runs deterministic valid and invalid temporary
fixtures for each enforced rule family and is executed by the workflow's plan
validation job.

## Consequences

- The plan gate fails only on rules it can evaluate repeatably.
- A green plan check cannot be mistaken for runtime qualification.
- Architecture review remains explicit instead of being replaced by noisy
  line-count heuristics.
