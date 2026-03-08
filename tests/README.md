## Test Layout

- `tests/unit/`: deterministic unit scopes.
  - `config/`: config parsing/IO tests (no CUDA).
  - `physics/`: physics solver tests (CUDA backend path).
- `tests/int/`: runtime integration scopes.
  - `protocol/`: backend protocol and control tests.
  - `runtime/`: bridge/runtime reconnect behavior.
  - `ui/`: Qt runtime integration.
- `tests/support/`: shared harness/helpers (`backend_harness`, `frontend_utils`, `poll_utils`, `physics_scenario`).
- `tests/data/`: deterministic fixtures.
- `tests/checks/check.py`: unified entrypoint for repository checks (`ini|mirror|no_legacy|launcher|quality|test_catalog|pr_policy|repo|python_quality|all`).
- `tests/checks/run.py`: generic dispatcher for catalog-driven gate/tool commands (`clang_tidy`, PR/IV&V/traceability gates).
- `tests/checks/catalog.json`: declarative command and check catalog used by the Python tooling.
- `tests/checks/policy_allowlist.txt`: explicit temporary file-size exceptions; each active group must be mirrored in `docs/quality/manifest/deviations.json`.

## Naming Conventions

- Add unit config tests in `tests/unit/config/*.cpp`.
- Add unit physics tests in `tests/unit/physics/*.cpp`.
- Add protocol tests in `tests/int/protocol/*.cpp`.
- Add runtime tests in `tests/int/runtime/*.cpp`.
- Add UI tests in `tests/int/ui/*.cpp`.

`tests/CMakeLists.txt` auto-discovers by directory (`file(GLOB ... CONFIGURE_DEPENDS)`), so adding a new test file in the right folder does not require editing CMake.

## Quality Gate

- Local: `python tests/checks/check.py all --root . --config simulation.ini`
- Make: `make check-all`
- Strict build: `cmake -S tests -B build-quality -G Ninja -DGRAVITY_PROFILE=prod -DGRAVITY_INTEGRATION_STRICT_WARNINGS=ON -DGRAVITY_INTEGRATION_ENABLE_SANITIZERS=ON`
- Static analyzer: `python tests/checks/run.py clang_tidy --root . --build-dir build-quality`
- Quality baseline docs: `python tests/checks/check.py quality --root .`
- Strict deterministic run: `ctest --test-dir build-quality --output-on-failure --timeout 180 --no-tests=error -R "TST_UNT_CONF_|TST_QLT_REPO_00(1|2|3|4|6|7)_"`
