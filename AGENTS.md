# AI Contribution Workflow

This file defines the standard procedure for AI agents working in this repository.

## Goal
For each task: pick one issue, implement it on a dedicated branch, merge to `main`, and close the issue.

## Mandatory Flow (Issue -> Branch -> Fix -> Merge -> Close)
1. Select one open issue (single issue per branch).
2. Sync local `main`.
3. Create a branch from `main`.
4. Implement the issue.
5. Run tests/checks.
6. Commit and push the branch.
7. Open a PR to `main` with `Closes #<issue_number>`.
8. Merge PR to `main`.
9. Ensure issue is closed.
10. Sync local `main` again.

## Commands (non-interactive)
```bash
git checkout main
git pull --ff-only origin main

git checkout -b issue/<N>-<slug>

# implement changes...
# run tests...

git add -A
git commit -m "feat(issue-<N>): <short summary>"
git push -u origin issue/<N>-<slug>

gh pr create \
  --base main \
  --head issue/<N>-<slug> \
  --title "Issue #<N>: <short summary>" \
  --body "Implements #<N>\n\nCloses #<N>"

gh pr merge --squash --delete-branch

git checkout main
git pull --ff-only origin main

Rules

    Never work directly on main.

    One branch should target one issue only.

    Do not use environment variables for runtime behavior unless an issue explicitly requires it.

    Keep commits focused and traceable to the issue number.

    Pushes to main must originate from a merged issue/<N>-<slug> pull request.

Space-Grade Quality Gate (Mandatory)

Treat this repository as high-assurance software (astrophysics/space simulation):

    No merge without a green quality gate in CI (pr-fast).

    No compiler warnings allowed (-Werror).

    No static analyzer critical findings (clang-tidy).

    Every automated test case must be listed in docs/quality/quality_manifest.json with a unique TST-... ID.

    Keep runtime behavior deterministic (fixed seeds in tests).

NASA-First Standards Profile (Mandatory)

    Primary compliance: NPR-7150.2D + NASA-STD-8739.8B.

    No recursion. No dynamic allocation after initialization. Fixed-bound loops only.

    Any standards-impacting change must update docs/quality/standards-profile.md.

File Size Policy

    Target length: <= 200 lines. Strong alert threshold: > 300 lines.

    Split files by responsibility. No "god files".

    Exceptions (generated code) must be documented in the PR.

C++ & CUDA Conventions (Strict PascalCase)

    Naming: PascalCase for all C++, CUDA, and Header files.

    1:1 Mapping: Filename MUST match the primary class name (e.g., OctreeSolver.cpp).

    Loi de Proximité: Private headers (.hpp) and implementation files (.cpp/.cu) stay in the same src/ directory.

    Public API: Only headers defining the public component interface go into the component's include/ directory.

    Fragments: CUDA kernel fragments use .inl and live in a fragments/ sub-directory.

    Namespaces: Single-level only (e.g., namespace bltzr_physics). No using namespace.

    Ownership: No raw pointers for ownership. Use std::unique_ptr.

    Const Correctness: Mandatory. Mark single-argument constructors as explicit.

Programming Languages Naming Rules

    C++ / CUDA / Qt: PascalCase.cpp, PascalCase.hpp.

    Rust / Python / Scripts: snake_case.rs, snake_case.py, snake_case.sh.

    Configuration: snake_case.ini, snake_case.json.

Test Organization Policy

    No tests in production source folders.

    All tests live under tests/.

    Organize tests by functional behavior, synchronized with quality_manifest.json.

Design Patterns

    Strategy: Interchangeable implementations behind interfaces.

    Dependency Injection: Pass dependencies via constructors.

    Observer: Decoupled event publishing to sinks.

    Composition: Prefer composition over inheritance. No Singletons.

If PR merge is not available
Bash

gh issue close <N> --comment "Implemented and merged to main."