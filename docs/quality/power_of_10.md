# Power of 10 Profile

This repository adopts Gerard Holzmann's `Power of 10` rules as a secondary coding-discipline profile for critical software work.

The profile is used in two modes:

- `automated`: checked by repository policy or CI tooling.
- `policy-only`: enforced through review, design constraints, and targeted analysis because the rule is not reliable to prove with a simple repository scan.

## Rule Mapping

1. `No complex flow control`
   Automation status: `partial`
   Automated checks:
   - production C++ paths must not use `goto`
   - production C++ paths must not use `setjmp` or `longjmp`
   - production C++ paths must not use `do { ... } while (...)`
   Policy note:
   - loop exits and state-machine transitions still require review for clarity

2. `All loops must have a fixed upper bound`
   Automation status: `partial`
   Automated checks:
   - production C++ paths must not use open-ended `while (true)` loops
   Policy note:
   - bounded loops must be justified in review
   - long-running service loops are allowed only at explicit process/runtime boundaries

3. `No dynamic allocation after initialization`
   Automation status: `policy-only`
   Policy note:
   - mission-critical `prod` paths should avoid runtime heap growth where practical
   - UI, plugin, and process-boundary setup code may allocate during startup

4. `Functions should be small`
   Automation status: `partial`
   Automated checks:
   - repository file-size policy remains enforced
   Policy note:
   - function size and decomposition remain review items

5. `Use at least two assertions per function on average`
   Automation status: `policy-only`
   Policy note:
   - invariants should be explicit in tests, validation code, and contracts

6. `Declare data objects at the smallest possible scope`
   Automation status: `policy-only`
   Policy note:
   - prefer narrow scope and immutable locals where practical

7. `Check the return value of non-void functions, or explicitly cast to void`
   Automation status: `partial`
   Automated checks:
   - strict analyzer lanes include `clang-tidy` ignored-return-value coverage
   - internal normalization/configuration status APIs are marked `[[nodiscard]]`
   Policy note:
   - silent error swallowing remains forbidden by review

8. `Use the preprocessor sparingly`
   Automation status: `partial`
   Automated checks:
   - headers must use strict include guards instead of `#pragma once`
   - C++ sources in the repository must not use preprocessor conditionals outside header include guards
   - C++ sources in the repository must not define macros outside header include guards
   Policy note:
   - platform/compiler variability must be isolated with build-system source selection or explicit runtime adapters, not conditional compilation

9. `Restrict pointer use to a single dereference, and do not use function pointers`
   Automation status: `partial`
   Automated checks:
   - repository policy restricts unnamed namespaces, `using`, and selected unsafe pointer patterns
   - production C++ paths must not introduce function pointer typedefs outside explicit ABI boundary headers
   - frontend/module ABI boundaries must encapsulate raw pointers immediately
   Policy note:
   - `using namespace`, symbol imports, and type aliases stay forbidden in repository C++ code to keep symbol provenance explicit during review
   - C ABI, Qt, OS, and plugin boundaries may require explicit pointer exceptions

10. `Compile with all warnings enabled; use one or more analyzers; keep the code warning-free`
    Automation status: `automated`
    Automated checks:
    - strict warning policy in CI
    - analyzer and repository quality gates in CI

## Repository Enforcement

The enforceable subset is wired through `python_tools/checks/repo_policy.py` and runs in the repository quality gate.

Current automated `Power of 10` repository checks cover:

- `goto` forbidden in production C++ paths
- `setjmp` and `longjmp` forbidden in production C++ paths
- `do { ... } while (...)` forbidden in production C++ paths
- `while (true)` forbidden in production C++ paths
- `#pragma once` forbidden; headers must use strict include guards
- preprocessor conditionals forbidden in repository C++ sources outside header include guards
- preprocessor macro definitions forbidden in repository C++ sources outside header include guards
- `using namespace`, symbol imports, and type aliases forbidden in repository C++ sources
- function pointer typedefs forbidden outside explicit ABI boundary headers

The remaining rules are tracked as review expectations and higher-assurance process constraints, not as naive regex-only repository failures.
