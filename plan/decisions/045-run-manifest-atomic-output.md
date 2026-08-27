# Decision 045: Run Manifest and Atomic Output Lifecycle

Status: accepted
Issue: #643
Plan version: 1.0.28

## Context

The binary snapshot codec can validate and materialize one frame, but it does
not define a complete run boundary. A caller needs a deterministic manifest,
an explicit output-directory preparation step, and a publication protocol that
never exposes a partially written state as a completed output. The first
implementation must remain single-rank and must not silently gather or
overwrite data.

## Decision

Materialize the run lifecycle under `src/io/metadata`:

- `MetadataRunInfo` is the value contract for versions, simulation settings,
  gravity, units, Barnes-Hut limits, deterministic generation, output policy,
  diagnostics policy, capabilities, and rank identity.
- `MetadataManifest` writes one fixed-order JSON document. It records the
  configuration and capabilities before the ordered list of completed state
  outputs. Wall-clock metadata is excluded.
- `MetadataRun::Prepare` is the only operation that creates the run root,
  `states/`, and `diagnostics/`. It accepts an absent or empty root and rejects
  a non-empty root, a file root, and unsupported distributed ranks.
- State and manifest files are written to a `.tmp` sibling and published with
  an atomic filesystem rename. Existing state targets and temporary siblings
  are rejected instead of overwritten.
- A state is added to the manifest only after its binary snapshot is published.
  If manifest publication fails, the new state is removed and the completed
  output list is rolled back.

The implementation consumes the existing `SnapshotFrameView` and
`SnapshotWriter` contracts. CLI orchestration, diagnostics data, restart, MPI
shards, and HDF5 remain separate work.

## Consequences

The first output lifecycle is deterministic, bounded, and safe to rerun only
against a fresh or empty root. A manifest is the authoritative list of
completed state files, while an unreferenced or temporary file is never
reported as a completed output. The contract is locally qualified by
`TST-P6-003`; cross-rank publication and full CLI integration remain
unqualified.
