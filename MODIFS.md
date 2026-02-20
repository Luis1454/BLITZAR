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

