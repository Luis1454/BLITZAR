# Backlog Modifs Simulation

Objectif: centraliser les modifs restantes et suivre l'avancement.

## Legende
- Priorite: `P0` critique, `P1` important, `P2` confort
- Statut: `[ ]` a faire, `[~]` en cours, `[x]` termine

## Architecture
- [ ] `P0` Stabiliser la separation backend/frontend (API claire et versionnee).
- [ ] `P1` Ajouter une interface de controle distante (socket/HTTP) pour piloter la simu.
- [ ] `P1` Uniformiser les points d'entree (`headless`, `sfml`, `qt`) sur la meme couche backend.
- [ ] `P2` Ajouter un mode plugin UI (charger un frontend externe).

## Simulation
- [ ] `P0` Finaliser l'octree GPU (perf + exactitude).
- [ ] `P0` Corriger la stabilite numerique en mode SPH (eviter l'explosion).
- [ ] `P1` Ajouter des scenarios de validation physique (orbite stable, choc, nuage).
- [ ] `P1` Ajouter des metriques auto (drift energie, temps/step, occupancy GPU).

## I/O et formats
- [ ] `P0` Documenter formellement le format de snapshot principal (`VTK`).
- [ ] `P1` Charger un snapshot directement depuis l'UI (drag/drop + bouton).
- [ ] `P1` Export incremental (autosave toutes N steps).
- [ ] `P2` Ajouter metadata et version de format dans les exports.

## UI / UX
- [ ] `P0` Garantir le refresh visuel SFML/Qt a cadence stable.
- [ ] `P1` Finaliser les vues multi-cameras (sync/unsync).
- [ ] `P1` Gimbal 3 axes: precision, sensibilite reglable, reset orientation.
- [ ] `P1` Graphes energie: zoom, pause, historique long.
- [ ] `P2` UI responsive (layouts petits ecrans).

## Build / DevEx
- [ ] `P0` Fiabiliser le build Qt sur machine propre (deps + runtime plugins).
- [ ] `P1` Ajouter presets CMake (`dev`, `release`, `profiling`).
- [ ] `P1` Script unique d'install des deps graphiques.
- [ ] `P2` Nettoyer warnings compilateur non bloquants.

## Tests
- [ ] `P0` Tests de non regression backend (steps deterministes sur seed fixe).
- [ ] `P1` Tests import/export (`vtk`, `xyz`) round-trip.
- [ ] `P1` Test perf minimal (budget FPS selon nombre de particules).
- [ ] `P2` CI locale: build + smoke test frontends.

## Notes libres
- Ajouter ici les idees, bugs reproduits, commandes de repro, captures.

## Plan refacto complet (sauvegarde etapes)
- [x] `P0` Reorganiser l'arborescence en domaines (`apps/`, `engine/`, `modules/`) avec chemins coherents.
- [x] `P0` Rebrancher le build sur la nouvelle arbo (`CMakeLists.txt` racine + `tests/CMakeLists.txt`).
- [x] `P0` Revalider compilation complete (`make build-run` et `make build-ci`).
- [~] `P0` Stabiliser les tests d'integration Qt (`QtMainWindowIntegration.*` crash `0xc0000409` a corriger).
- [ ] `P0` Decouper le CMake monolithique en modules (`cmake/targets/*.cmake`).
- [ ] `P0` Creer des librairies de domaine pour supprimer la redondance de sources:
  - `gravityEngine` (backend + config + protocol + physics)
  - `gravityFrontendShared`
  - `gravityQtUi`
- [ ] `P1` Introduire une couche CUDA dediee (object lib ou lib specialisee) pour eviter recompilations inutiles.
- [ ] `P1` Restreindre les includes par cible (supprimer les include dirs trop globaux).
- [ ] `P1` Unifier la declaration des tests (eviter logique double root/tests-integration).
- [ ] `P1` Nettoyer les repertoires legacy devenus vides (`src/`, `include/`, `fragments/`) apres validation finale.
- [ ] `P1` Mettre a jour la doc build/run avec la nouvelle architecture (README + scripts + exemples de commandes).

Ordre d'execution recommande:
1. Corriger `QtMainWindowIntegration.*` (stabilite runtime).
2. Extraire le CMake en modules.
3. Introduire les libs de domaine.
4. Nettoyer includes/cibles et supprimer la redondance.
5. Purger les dossiers legacy vides et finaliser la doc.

## Journal conformite NASA-first

### 2026-02-28 - V1+V2+V3 (policy/CI/gouvernance)

- Exigences touchees: `REQ-QUAL-001`, `REQ-QUAL-002`, `REQ-COMP-001`, `REQ-COMP-002`, `REQ-TEST-001`.
- Profil cible: `prod` en lanes evidence CI (PR/nightly/release).
- Statut analyseur local: `clang-tidy` non disponible localement (check outille en CI).
- Tests executes:
  - `python tests/checks/check.py all --root . --config simulation.ini` (OK)
  - `cmake -S tests -B build-quality -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DGRAVITY_INTEGRATION_STRICT_WARNINGS=ON -DGRAVITY_INTEGRATION_ENABLE_SANITIZERS=ON` (OK)
  - `cmake --build build-quality --parallel` (OK)
  - `ctest --test-dir build-quality --output-on-failure --timeout 180 --no-tests=error -R "ConfigArgsTest\\.TST_UNT_CONF_|TST_QLT_REPO_00(1|2|3|4|6)_"` (OK)
  - `make build-prod` (OK)

### 2026-02-28 - V3 (reduction cible >200, lot tests/checks)

- Exigences touchees: `REQ-QUAL-001`, `REQ-TEST-001`.
- Profil cible: `prod`.
- Refactors:
  - split `tests/checks/quality_check.py` + extraction `tests/checks/quality_check_common.py`.
  - split `tests/cmake/setup.cmake` + extraction `tests/cmake/windows_paths.cmake`.
  - split `tests/unit/config/args_cli.cpp` + extraction `tests/unit/config/args_cli_usage.cpp`.
  - split `tests/support/backend_harness.cpp` + extraction `tests/support/backend_harness_runtime.cpp`.
  - trim `apps/launcher/main.cpp` to pass target size policy (`<=200` lines).
- Tests executes:
  - `python tests/checks/check.py all --root . --config simulation.ini` (OK)
  - `cmake -S tests -B build-quality -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DGRAVITY_PROFILE=prod -DGRAVITY_INTEGRATION_STRICT_WARNINGS=ON -DGRAVITY_INTEGRATION_ENABLE_SANITIZERS=ON` (OK)
  - `cmake --build build-quality --parallel` (OK)
  - `ctest --test-dir build-quality --output-on-failure --timeout 180 --no-tests=error -R "ConfigArgsTest\\.TST_UNT_CONF_|TST_QLT_REPO_00(1|2|3|4|6)_"` (OK)

### 2026-02-28 - V3 (reduction cible >200, lot backend/module host)

- Exigences touchees: `REQ-QUAL-001`, `REQ-COMP-002`, `REQ-TEST-001`.
- Profil cible: `prod`.
- Refactors:
  - split backend daemon args/signaux:
    - `apps/backend-service/main.cpp`
    - `apps/backend-service/backend_args.hpp`
    - `apps/backend-service/backend_args.cpp`
  - split module host dynamic loading path:
    - `runtime/src/frontend/FrontendModuleHandle.cpp`
    - `runtime/src/frontend/FrontendModuleHandleLoad.cpp`
    - `runtime/src/frontend/FrontendModuleHandleInternal.hpp`
    - mirror header `runtime/include/frontend/FrontendModuleHandleLoad.hpp`
  - update build wiring:
    - `cmake/apps.cmake` (new sources for backend daemon and module host).
- Resultat policy:
  - `apps/backend-service/main.cpp` passe de 277 -> 124 lignes.
  - `runtime/src/frontend/FrontendModuleHandle.cpp` passe de 264 -> 111 lignes.
  - warnings >200 elimines pour ces deux zones.
- Tests executes:
  - `python tests/checks/check.py all --root . --config simulation.ini` (OK)
  - `cmake -S tests -B build-quality -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DGRAVITY_PROFILE=prod -DGRAVITY_INTEGRATION_STRICT_WARNINGS=ON -DGRAVITY_INTEGRATION_ENABLE_SANITIZERS=ON` (OK)
  - `cmake --build build-quality --parallel` (OK)
  - `ctest --test-dir build-quality --output-on-failure --timeout 180 --no-tests=error -R "ConfigArgsTest\\.TST_UNT_CONF_|TST_QLT_REPO_00(1|2|3|4|6)_"` (OK)
  - `make build-prod` (OK)

### 2026-02-28 - V3+Gouvernance (issue 106 + issue 112)

- Exigences touchees: `REQ-COMP-002`, `REQ-QUAL-001`, `REQ-TEST-001`.
- Profil cible: `prod`.
- Changements gouvernance/CI:
  - ajout du check PR bloquant `tests/checks/pr_policy_check.py` + self-test `tests/checks/pr_policy_check_test.py` et fixtures.
  - ajout du job `PR Policy Guard` dans `.github/workflows/pr-fast.yml`.
  - ajout du test normalise `TST_QLT_REPO_007_PrPolicyCheck` (targets, catalogues, traceability, requirements, crosswalk).
  - mise a jour des regex CI pour inclure `007` dans PR/nightly/release.
- Reduction dette >200:
  - split `Makefile` vers `make/check.mk` et `make/runtime.mk`.
  - split `cmake/core.cmake` vers `cmake/core/{profile,toolchain,targets}.cmake`.
  - compression/simplification `docs/README_full.md`.
  - split `modules/qt/ui/ParticleView.cpp` avec `modules/qt/ui/ParticleViewColor.cpp` + `modules/qt/include/ui/ParticleViewColor.hpp`.
  - reduction `engine/src/platform/posix/SocketPlatformPosix.cpp`.
  - reduction `engine/src/platform/win/SocketPlatformWin.cpp`.
  - split `engine/src/physics/cuda/fragments/OctreeImpl.inl` vers `OctreeBuild.inl` et `OctreeForce.inl`.
  - reduction `.github/workflows/nightly-full.yml`.
  - resultat: `python tests/checks/check.py repo --root . --config simulation.ini` sans warning policy (>200 elimines).
- Tests executes:
  - `python tests/checks/pr_policy_check_test.py` (OK)
  - `python tests/checks/check.py all --root . --config simulation.ini` (OK)
  - `cmake -S tests -B build-quality -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DGRAVITY_PROFILE=prod -DGRAVITY_INTEGRATION_STRICT_WARNINGS=ON -DGRAVITY_INTEGRATION_ENABLE_SANITIZERS=ON` (OK)
  - `cmake --build build-quality --parallel` (OK)
  - `ctest --test-dir build-quality --output-on-failure --timeout 180 --no-tests=error -R "ConfigArgsTest\\.TST_UNT_CONF_|TST_QLT_REPO_00(1|2|3|4|6|7)_"` (OK)
  - `make build-prod` (OK)
  - `python tests/checks/clang_tidy_check.py --root . --build-dir build-quality` (KO local: `clang-tidy` absent)

### 2026-03-01 - Refonte checks Python (core + pytest + TST_008)

- Exigences touchees: `REQ-COMP-002`, `REQ-TEST-001`, `REQ-COMP-001`, `REQ-QUAL-001`.
- Profil cible: `prod`.
- Refactors:
  - creation d'un noyau modulaire `tests/checks/core/`:
    - `models.py`, `reporting.py`, `io_utils.py`, `repo_policy.py`, `quality_baseline.py`, `test_catalog.py`, `pr_policy.py`, `runner.py`
  - transformation des scripts publics en wrappers CLI minces:
    - `tests/checks/check.py`
    - `tests/checks/quality_check.py`
    - `tests/checks/test_catalog_check.py`
    - `tests/checks/pr_policy_check.py`
  - maintien compat import via shim `tests/checks/quality_check_common.py`
  - ajout de tests unitaires Python `pytest`:
    - `tests/checks/tests/test_runner.py`
    - `tests/checks/tests/test_repo_policy.py`
    - `tests/checks/tests/test_quality_baseline.py`
    - `tests/checks/tests/test_test_catalog.py`
    - `tests/checks/tests/test_pr_policy.py`
  - ajout du test catalogue `TST_QLT_REPO_008_PyChecksUnit`:
    - `tests/cmake/targets.cmake` (ctest -> `python -m pytest -q tests/checks/tests`)
    - update regex lanes CI PR/nightly/release pour inclure `008`
  - synchro baseline qualite:
    - `docs/quality/test_catalog.csv`
    - `docs/quality/traceability.csv`
    - `docs/quality/requirements.json`
- Tests executes:
  - `python tests/checks/check.py all --root . --config simulation.ini` (OK)
  - `python -m pytest -q tests/checks/tests` (OK, `14 passed`)
  - `cmake -S tests -B build-quality -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DGRAVITY_PROFILE=prod -DGRAVITY_INTEGRATION_STRICT_WARNINGS=ON -DGRAVITY_INTEGRATION_ENABLE_SANITIZERS=ON` (OK)
  - `cmake --build build-quality --parallel` (OK)
  - `ctest --test-dir build-quality --output-on-failure --timeout 180 --no-tests=error -R "ConfigArgsTest\\.TST_UNT_CONF_|TST_QLT_REPO_00(1|2|3|4|6|7|8)_"` (OK)
  - `make build-prod` (OK)

### 2026-03-01 - Refonte Python class-first complete (python_tools + TST_009)

- Exigences touchees: `REQ-COMP-002`, `REQ-TEST-001`, `REQ-COMP-001`, `REQ-QUAL-001`.
- Profil cible: `prod`.
- Refactors:
  - ajout du package commun `python_tools/`:
    - `python_tools/core/{contracts,models,base_check,base_command,runner,reporting,io,errors,typing_ext}.py`
    - `python_tools/policies/{ini_check,mirror_check,no_legacy_check,launcher_check,repo_policy,quality_baseline,test_catalog,pr_policy}.py`
    - `python_tools/ci/{clang_tidy,coverage_dashboard,release_bundle,python_quality_gate}.py`
  - migration des wrappers `tests/checks/*.py` vers des wrappers CLI minces classes (`BaseCliCommand`).
  - migration scripts CI:
    - `scripts/ci/nightly/build_dashboard.py`
    - `scripts/ci/release/package_bundle.py`
  - ajout outillage Python qualite via `pyproject.toml` (`pytest`, `ruff`, `mypy`).
  - suppression du core legacy `tests/checks/core/*`.
  - ajout test normalise `TST_QLT_REPO_009_PythonQualityGate` (`pytest + ruff + mypy`).
  - synchro baseline qualite:
    - `docs/quality/test_catalog.csv`
    - `docs/quality/traceability.csv`
    - `docs/quality/requirements.json`
  - updates CI PR/Nightly/Release:
    - installation `pytest ruff mypy`
    - execution `ruff check` et `mypy`
    - regex ctest `TST_QLT_REPO_00(...|9)_`.
- Tests executes:
  - `python tests/checks/check.py all --root . --config simulation.ini` (OK)
  - `python -m pytest -q tests/checks/tests` (OK)
  - `python -m ruff check .` (OK)
  - `python -m mypy tests/checks scripts/ci python_tools` (OK)
  - `cmake -S tests -B build-quality -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DGRAVITY_PROFILE=prod -DGRAVITY_INTEGRATION_STRICT_WARNINGS=ON -DGRAVITY_INTEGRATION_ENABLE_SANITIZERS=ON` (OK)
  - `cmake --build build-quality --parallel` (OK)
  - `ctest --test-dir build-quality --output-on-failure --timeout 180 --no-tests=error -R "ConfigArgsTest\\.TST_UNT_CONF_|TST_QLT_REPO_00(1|2|3|4|6|7|8|9)_"` (OK)
  - `make build-prod` (OK)



### 2026-03-01 - Split `SimulationArgs` (hard-limit remediation lot)

- Exigences touchees: `REQ-QUAL-001`, `REQ-TEST-001`.
- Profil cible: `prod`.
- Refactors:
  - split `engine/src/config/SimulationArgs.cpp` en modules cohésifs:
    - `SimulationArgsParse.cpp`
    - `SimulationArgsCoreOptions.cpp`
    - `SimulationArgsFrontendOptions.cpp`
    - `SimulationArgsInitStateOptions.cpp`
    - `SimulationArgsFluidOptions.cpp`
    - `SimulationArgsInitOptions.cpp` (agrégateur)
  - ajout des headers miroir correspondants sous `engine/include/config/`.
  - update build wiring:
    - `cmake/core/toolchain.cmake`
    - `tests/cmake/targets.cmake`
  - nettoyage allowlist obsolète:
    - `tests/checks/policy_allowlist.txt` (suppression `engine/src/config/SimulationArgs.cpp`).
- Tests executes:
  - `python tests/checks/check.py all --root . --config simulation.ini` (OK)
  - `cmake -S tests -B build-quality -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DGRAVITY_PROFILE=prod -DGRAVITY_INTEGRATION_STRICT_WARNINGS=ON -DGRAVITY_INTEGRATION_ENABLE_SANITIZERS=ON` (OK)
  - `cmake --build build-quality --parallel` (OK)
  - `ctest --test-dir build-quality --output-on-failure --timeout 180 --no-tests=error -R "ConfigArgsTest\\.TST_UNT_CONF_"` (OK)
