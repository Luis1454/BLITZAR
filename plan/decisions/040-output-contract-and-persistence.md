# Decision 040: Deterministic Output Contract

Status: accepted
Issue: #639
Plan version: 1.0.24

## Context

The configured CLI run proves that the rewrite can execute a bounded
simulation, but its first/last particle values are only smoke diagnostics. The
repository also contains a versioned SnapshotHeader contract without a file
codec. A persistence design is required before materializing src/io, and it
must not couple file ownership to numerical state or introduce a hidden MPI
gather.

## Decision

Freeze one rewrite-owned output contract in `plan/output_contract.json`.

The first implementation accepts two optional directives:

- `output(directory, every_steps, write_initial, write_final)`;
- `diagnostics(every_steps, energy, momentum, relative_error)`.

Relative output paths are resolved from the configuration file directory. A
non-empty destination is rejected by default. The deterministic run layout is:

```text
run/
  manifest.json
  states/state-00000000.bin
  diagnostics/conservation.csv
```

Binary snapshot version 1 uses little-endian fixed-width values, IEEE-754
binary64 scalars, the `BZRS` magic, an FNV-1a-64 payload checksum, and the
following SoA payload order: ids, position x/y/z, velocity x/y/z, and mass.
The codec must encode fields explicitly rather than writing C++ object memory.

The initial runtime implementation is single-rank. A multi-rank output path
must use explicit shards and a manifest or return `BLITZAR_STATUS_UNSUPPORTED`;
it must not silently gather the complete state. HDF5 remains optional and
deferred. The public C and C++ SDK is not expanded by this contract.

Manifest metadata has a fixed field order and excludes wall-clock timestamps.
Snapshot publication uses a temporary file followed by atomic rename. Initial,
periodic, and final writes are defined so that a final step is not duplicated
when it already matches the configured interval.

P6 persistence work depends on the qualified P2 and P3 contracts. It does not
depend on the unrelated deferred PM/TreePM root in P5.

## Consequences

- The console summary can report execution and artifact counts instead of
  arbitrary particle samples.
- Snapshot and diagnostics writers can consume const spans without owning
  simulation buffers.
- Corrupt, truncated, incompatible, and checksum-invalid files have a
  deterministic rejection contract before any destination state changes.
- Binary persistence remains deferred until `src/io` and its runtime evidence
  are materialized.
- MPI, HIP, and HDF5 integrations require separate qualification issues and
  cannot change the single-rank format implicitly.
