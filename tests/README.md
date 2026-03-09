## Test Layout

- Product verification:
- `tests/unit/`: deterministic unit scopes.
  - `config/`: config parsing/IO tests (no CUDA).
  - `physics/`: physics solver tests (CUDA backend path).
- `tests/int/`: runtime integration scopes.
  - `protocol/`: backend protocol and control tests.
  - `runtime/`: bridge/runtime reconnect behavior.
  - `ui/`: Qt runtime integration.
- `tests/support/`: shared harness/helpers (`backend_harness`, `frontend_utils`, `poll_utils`, `physics_scenario`).
- `tests/data/`: deterministic fixtures.
- Quality tooling:
- `tests/checks/check.py`: unified entrypoint for repository checks (`ini|mirror|no_legacy|launcher|quality|test_catalog|pr_policy|repo|python_quality|all`).
- `tests/checks/run.py`: generic dispatcher for catalog-driven gate/tool commands (`clang_tidy`, PR/IV&V/traceability gates).
- `tests/checks/catalog.json`: declarative command and check catalog used by the Python tooling.
- `tests/checks/policy_allowlist.txt`: explicit temporary file-size exceptions; each active group must be mirrored in `docs/quality/manifest/deviations.json`.
- `python_tools/`: shared implementation for policies, CI packagers, and quality support code.

## Naming Conventions

- Add unit config tests in `tests/unit/config/*.cpp`.
- Add unit physics tests in `tests/unit/physics/*.cpp`.
- Add protocol tests in `tests/int/protocol/*.cpp`.
- Add runtime tests in `tests/int/runtime/*.cpp`.
- Add UI tests in `tests/int/ui/*.cpp`.

`tests/CMakeLists.txt` auto-discovers by directory (`file(GLOB ... CONFIGURE_DEPENDS)`), so adding a new test file in the right folder does not require editing CMake.

## Quality Gate

- Local repository gate: `make quality-local`
- Python style/type gate: `make quality-python`
- Strict CI-aligned preflight: `make quality-strict`
- Strict build only: `make quality-configure quality-build`
- Static analyzer only: `make quality-analyze`
- Quality baseline docs: `python tests/checks/check.py quality --root .`
- Strict deterministic run only: `make quality-test`
