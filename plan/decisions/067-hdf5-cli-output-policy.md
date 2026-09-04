# Decision 067: Capability-Gated HDF5 CLI Output

- Status: active
- Scope: clean-room issue #684
- Plan version: 1.0.51

## Decision

The configured CLI accepts an optional `format` argument on the `output`
directive. `binary` remains the default and preserves the existing output
layout. `hdf5` selects the versioned HDF5 adapter from Decision 062 for each
single-rank state file. The manifest records the selected format and every
completed output path uses the matching `.bin` or `.h5` extension.

The selection is an output configuration policy, not a public SDK or ABI
change. HDF5 output, restart, and post-processing remain single-rank and are
available only when the optional dependency is compiled into the target. If it
is unavailable, output preparation returns `BLITZAR_STATUS_UNSUPPORTED` before
creating the run directory; the binary format remains independently usable.

Restart and post-processing select their reader from the source manifest, so a
run cannot silently mix state extensions. HDF5 publication remains temporary
file plus atomic rename, and HDF5 reads retain bounded staging and transactional
state commit from Decision 062. Existing configurations without `format` are
deterministically equivalent to `format=binary`.

## Acceptance

`TST-P2-008` validates parsing, defaulting, and rejection of unknown formats.
`TST-P6-014` validates CLI HDF5 selection, manifest/path parity, HDF5 state
decoding when available, the pre-preparation unsupported boundary when absent,
and the unchanged binary fallback. The existing HDF5 adapter qualification
`TST-P6-012` remains the schema and corruption oracle.
