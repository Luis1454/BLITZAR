# AI Contribution Workflow (NASA-First Policy)

This file defines the standard procedure for AI agents working in the BLITZAR repository.

## Goal
Implement one issue per branch, maintain space-grade code quality, and ensure structural integrity by eliminating redundant naming and enforcing strict encapsulation.

## Mandatory Flow (Issue -> Branch -> Fix -> Merge -> Close)
1. Select one open issue (one issue per branch).
2. Sync local `main` (`git pull --ff-only origin main`).
3. Create branch: `issue/<N>-<slug>`.
4. Implement, test, and commit using the standard CLI workflow.
5. Open PR with `Closes #<N>`.
6. Merge via `gh pr merge --squash --delete-branch`.
7. Sync local `main` again.

## Commands (non-interactive)
```bash
git checkout main && git pull --ff-only origin main
git checkout -b issue/<N>-<slug>
# ... implement & test ...
git add -A && git commit -m "feat(issue-<N>): <short summary>"
git push -u origin issue/<N>-<slug>
gh pr create --title "Issue #<N>: <summary>" --body "Closes #<N>"
gh pr merge --squash --delete-branch

Space-Grade Quality Gate

    Zero Warnings: -Werror policy enforced.

    Static Analysis: clang-tidy critical findings must be resolved.

    Traceability: Every automated test case must be in docs/quality/quality_manifest.json with a TST-... ID.

    Determinism: Runtime behavior must be deterministic (fixed seeds for simulations/tests).

    Formatting: Every C++ file must be formatted using the project's .clang-format (Stroustrup style, 4 spaces, no tabs).

NASA-First Standards (NPR-7150.2D)

    No Recursion / No Dynamic Allocation after initialization.

    Fixed-bound loops only.

    Any standards-impacting change must update docs/quality/standards-profile.md.

File Size Policy

    Target length: <= 200 lines. Strong alert: > 300 lines.

    Split files by responsibility. Avoid "God files" or "Tool/Utils" catch-alls.

C++ & CUDA Conventions (Strict PascalCase)

    Naming: PascalCase for all C++, CUDA, and Header files.

    Anti-Redundancy: Do NOT repeat the component name in the filename.

        Bad: src/physics/PhysicsScenario.cpp

        Good: src/physics/Scenario.cpp

    1:1 Mapping: Filename MUST match the primary class name (e.g., OctreeSolver.cpp).

    Loi de Proximité (Strict Encapsulation):

        Public API: ONLY headers imported by OTHER components go into the include/ directory.

        Private Logic: All other headers and implementations stay in src/.

        No Parity Requirement: A header can have multiple .cpp helpers in src/ without needing dedicated internal headers for each.

    Fragments: CUDA kernel fragments use .inl in a fragments/ sub-directory.

    Namespaces: Single-level only (e.g., namespace bltzr_physics). No using namespace.

    Ownership: No raw pointers for ownership. Use std::unique_ptr by default.

    Const Correctness: Mandatory. Mark single-argument constructors as explicit.

Programming Languages Naming Rules

    C++ / CUDA / Qt: PascalCase.cpp, PascalCase.hpp (e.g., NumericalValidator.cpp).

    Rust / Python / Scripts: snake_case.rs, snake_case.py (e.g., quality_gate.py).

    Configuration: snake_case.ini, snake_case.json.

Design Patterns

    Strategy & DI: Interchangeable implementations behind interfaces. Pass dependencies via constructors.

    Composition: Prefer composition over inheritance.

    Singletons: Forbidden (except for strictly stateless utilities).

If PR merge is not available
Bash

gh issue close <N> --comment "Implemented and merged to main."