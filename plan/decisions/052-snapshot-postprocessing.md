# Decision 052: Snapshot Post-Processing Boundary

- Status: accepted
- Issue: #648
- Plan version: 1.0.36

## Context

BLITZAR persists deterministic single-rank binary snapshots and a run manifest.
Users need to recompute conservation diagnostics without rerunning the physical
simulation. The post-processing path must therefore consume persisted artifacts
as immutable input and must not create a second diagnostic format or a second
definition of the conservation metrics.

## Decision

The CLI exposes the explicit mode:

```text
blitzar_cli --post-process <run-directory>
```

The mode reads only `manifest.json` and the files named by its
`completed_outputs` list. The manifest is authoritative, and the input is
accepted only when all of the following hold:

- the run directory, manifest, and `states` directory exist;
- the manifest validates against the current product and plan versions;
- the snapshot format is version 1, single-rank, and compatible with the
  manifest configuration;
- completed steps are non-empty, strictly increasing, within the configured
  run bounds, and use the frozen `states/state-%08d.bin` names;
- the states directory contains exactly the listed regular files, with no
  temporary, extra, missing, or symlink entries;
- every snapshot header, payload, checksum, particle count, step, time, and
  distribution value is valid.

The post-processing implementation reuses `ConservationMetrics` for numerical
reduction and `ConservationCsv` for serialization. The CSV schema is fixed to
the eleven columns in `plan/output_contract.json`, uses the classic locale,
binary64-compatible precision 17, and default floating-point formatting.
Relative energy and momentum errors use the first emitted diagnostic record as
their reference. A disabled metric is serialized as the literal `nan`.

When diagnostics are enabled, post-processing emits only persisted steps whose
step is divisible by `diagnostics.every_steps`; when diagnostics are disabled,
every valid persisted snapshot is emitted. Online diagnostics use the same
checkpoint selection and writer, so a post-processed run and its source online
diagnostics file must be byte-identical for the same snapshots.

Output is prepared in `postProcessing/conservation.csv.tmp` and published by
rename to `postProcessing/conservation.csv` only after every input snapshot has
been validated and processed. A non-empty existing `postProcessing` directory
is rejected, and failed processing removes the temporary artifact without
altering the source run.

## Consequences

The feature is headless and has no GUI, visualization, or format-conversion
responsibility. It does not widen the public C ABI or expose persistence
internals through `blitzar.hpp`. MPI/HIP shard aggregation and HDF5 input remain
separate capabilities and are not implied by this single-rank mode.

The strict current-version policy prevents a post-processor from silently
interpreting a run generated under another frozen contract. Future format
versions require an explicit compatibility decision and a new acceptance test.
