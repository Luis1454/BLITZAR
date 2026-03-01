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

Replace:
- `<N>` = issue number
- `<slug>` = short kebab-case summary

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
```

## Rules

- Never work directly on `main`.
- One branch should target one issue only.
- Do not use environment variables for runtime behavior unless an issue explicitly requires it.
- Keep commits focused and traceable to the issue number.
- Prefer automatic issue closure through PR body: `Closes #<N>`.

## Space-Grade Quality Gate (Mandatory)

Treat this repository as high-assurance software (astrophysics/space simulation):

- No merge without a green quality gate in CI (`pr-fast`).
- No compiler warnings allowed in strict lanes (`-Werror`/`/WX` policy).
- No static analyzer critical findings in strict lanes (`clang-tidy` analyzer checks).
- All deterministic tests must pass (`ctest`), including integration-safe contracts.
- Repository policy checks must pass (naming, extension policy, file-size policy, no legacy tokens).
- Keep runtime behavior deterministic where possible (fixed seeds/timeouts in tests).
- Any temporary exception (file-size or policy) must be explicit and traceable in repo policy allowlists.
- Quality baseline artifacts in `docs/quality/` must stay synchronized (`requirements.json`, `traceability.csv`, `test_catalog.csv`, `standards_profile.md`, `nasa_crosswalk.csv`, FMEA, IV&V, tool qualification, numerical validation).
- Every automated test case must be listed in `docs/quality/test_catalog.csv` with a unique normalized code (`TST-...` format).

## NASA-First Standards Profile (Mandatory)

- Primary compliance posture is NASA-first: `NPR-7150.2D` + `NASA-STD-8739.8B`.
- Keep ECSS mappings as secondary crosswalk context, not primary governance.
- Preserve `prod` vs `dev` separation:
  - `prod`: deterministic qualification evidence path and strict CI gates.
  - `dev`: exploratory path; not qualification evidence unless reproduced under `prod` constraints.
- Do not introduce dynamic reload behavior in mission-critical runtime paths in `prod`.
- Any standards-impacting change must update:
  - `docs/quality/standards_profile.md`
  - `docs/quality/test_catalog.csv`
  - `docs/quality/nasa_crosswalk.csv`
  - `docs/quality/requirements.json`
  - `docs/quality/traceability.csv`

Required local pre-flight before opening PR:

```bash
python tests/checks/check.py all --root . --config simulation.ini
cmake -S tests -B build-quality -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DGRAVITY_INTEGRATION_STRICT_WARNINGS=ON \
  -DGRAVITY_INTEGRATION_ENABLE_SANITIZERS=ON
cmake --build build-quality --parallel
python tests/checks/clang_tidy_check.py --root . --build-dir build-quality
ctest --test-dir build-quality --output-on-failure --timeout 180 -R "ConfigArgsTest\\.TST_UNT_CONF_|TST_QLT_REPO_00(1|2|3|4|6)_"
```

`integration_real` tests require a real backend executable and run in dedicated lanes where backend artifacts are present.

## File Size Policy (All Files)

- Apply this policy to every file type (`.cpp`, `.hpp`, `.cu`, `.cmake`, `.md`, scripts, etc.).
- Target file length: `<= 200` lines.
- Hard limit: `<= 300` lines.
- If a file grows past the hard limit, split it by responsibility immediately.
- Allowed exceptions: generated files, vendored third-party code, or performance-critical fragments; document the reason in the related PR/issue.

## C++ Conventions

- Use `.hpp` and `.cpp` files only for C++ sources.
- Put declarations in `.hpp` files and definitions in `.cpp` files.
- Do not place class/function definitions in headers.
- Prefer a 1:1 mapping: one `.cpp` file per `.hpp` file.
- Prefer one class per file.
- Headers must be self-contained and compile on their own.
- Do not use `using namespace` in headers.
- Keep includes minimal and use forward declarations when possible.
- Keep include order stable: standard library, third-party, then project headers.
- Mark single-argument constructors as `explicit`.
- Use `override` for overridden virtual methods (`final` when appropriate).
- Enforce strict `const` correctness.
- Prefer `const T&` for heavy inputs and clear return-value semantics.
- Make ownership explicit: default to `std::unique_ptr`, use `std::shared_ptr` only when required.
- Avoid mutable global state.
- Keep one error-handling strategy per module (typed status or exceptions), avoid mixing ad hoc styles.
- New production classes should come with unit tests.
- No unnamed namespace, prefer static functions instead.
- No global functions or variables.

## Test Organization Policy (NASA-First)

- Do not place automated tests in production source files (`apps/`, `engine/`, `runtime/`, `modules/`).
- Keep all automated tests under `tests/` only.
- Organize tests by responsibility, not by one-test-per-file:
  - Prefer one test file per functional behavior/module.
  - Keep multiple related test cases in the same file when they validate the same requirement family.
- Use normalized test IDs in names (`TST_...`) and keep them synchronized with:
  - `docs/quality/test_catalog.csv`
  - `docs/quality/requirements.json`
  - `docs/quality/traceability.csv`
- For high-assurance evidence quality (NASA-first profile), maintain strict separation:
  - production code artifacts
  - verification artifacts (tests, test helpers, quality checks)

## Design Patterns

- Strategy: use interchangeable implementations behind interfaces (`IIqSource`, `IByteSource`, `IDemodulator`, `IFrameSink`).
- Factory: select concrete backends from config using factories/registries instead of `if/else` chains.
- Adapter: wrap hardware/vendor APIs behind project interfaces.
- Builder: assemble pipeline graphs by mode (`iq` or `bytes`) in one composition layer.
- Dependency Injection: pass dependencies via constructors; avoid hidden service locators.
- Observer: publish frames/metrics/events to sinks without tight coupling.
- Null Object: use explicit no-op implementations only for tests or optional paths.
- Prefer composition over inheritance unless polymorphism is required by interface contracts.
- Avoid Singleton except for strictly stateless utilities.
- Keep pattern usage pragmatic: no pattern without a concrete maintenance or extensibility benefit.

## If PR merge is not available

Use manual close after pushing to `main`:

```bash
gh issue close <N> --comment "Implemented and merged to main."
```
