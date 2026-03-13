# Backlog Modifs Simulation

Objectif: centraliser les modifs restantes et suivre l'avancement.

## Legende
- Priorite: `P0` critique, `P1` important, `P2` confort
- Statut: `[ ]` a faire, `[~]` en cours, `[x]` termine

## Architecture
- [ ] `P0` Stabiliser la separation server/client (API claire et versionnee).
- [ ] `P1` Ajouter une interface de controle distante (socket/HTTP) pour piloter la simu.
- [ ] `P1` Uniformiser les points d'entree (`headless`, `qt`) sur la meme couche server.
- [ ] `P2` Ajouter un mode plugin UI (charger un client externe).

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
- [ ] `P0` Garantir le refresh visuel Qt a cadence stable.
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
- [ ] `P0` Tests de non regression server (steps deterministes sur seed fixe).
- [ ] `P1` Tests import/export (`vtk`, `xyz`) round-trip.
- [ ] `P1` Test perf minimal (budget FPS selon nombre de particules).
- [ ] `P2` CI locale: build + smoke test clients.

## Notes libres
- Ajouter ici les idees, bugs reproduits, commandes de repro, captures.

## Plan refacto complet
- [x] `P0` Reorganiser l'arborescence en domaines (`apps/`, `engine/`, `modules/`) avec chemins coherents.
- [x] `P0` Rebrancher le build sur la nouvelle arbo (`CMakeLists.txt` racine + `tests/CMakeLists.txt`).
- [x] `P0` Revalider compilation complete (`make build-run` et `make build-ci`).
- [~] `P0` Stabiliser les tests d'integration Qt (`QtMainWindowIntegration.*` crash `0xc0000409` a corriger).
- [ ] `P0` Decouper le CMake monolithique en modules (`cmake/targets/*.cmake`).
- [ ] `P0` Creer des librairies de domaine pour supprimer la redondance de sources.
- [ ] `P1` Introduire une couche CUDA dediee (object lib ou lib specialisee) pour eviter recompilations inutiles.
- [ ] `P1` Restreindre les includes par cible (supprimer les include dirs trop globaux).
- [ ] `P1` Unifier la declaration des tests (eviter logique double root/tests-integration).
- [ ] `P1` Nettoyer les repertoires legacy devenus vides (`src/`, `include/`, `fragments/`) apres validation finale.
- [ ] `P1` Mettre a jour la doc build/run avec la nouvelle architecture (README + scripts + exemples de commandes).

Ordre d'execution recommande:
1. Corriger `QtMainWindowIntegration.*`.
2. Extraire le CMake en modules.
3. Introduire les libs de domaine.
4. Nettoyer includes/cibles et supprimer la redondance.
5. Purger les dossiers legacy vides et finaliser la doc.

## Historique
- Le journal complet de conformite et des lots de refactorisation est archive dans `docs/history/modifs_journal_2026-02_03.md`.
