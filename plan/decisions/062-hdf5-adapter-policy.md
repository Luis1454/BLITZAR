# Decision 062: Optional HDF5 Adapter Policy

- Status: active
- Scope: clean-room issue #650
- Plan version: 1.0.46

## Decision

The binary snapshot codec remains the canonical CLI persistence format. HDF5 is
an optional internal adapter for the frozen `SnapshotFrameView` contract, not a
second public SDK or CLI execution path.

The adapter is selected at configure time with `BLITZAR_HDF5_MODE=AUTO|ON|OFF`:

- `AUTO` enables HDF5 when the C dependency is available and otherwise keeps the
  binary-only build.
- `ON` requires HDF5 and fails configuration when it is unavailable.
- `OFF` disables HDF5 explicitly.

The schema is version 1. It stores header attributes and the ordered SoA
datasets under `/particles`: `ids`, `position_x`, `position_y`, `position_z`,
`velocity_x`, `velocity_y`, `velocity_z`, and `mass`. Numeric payload bytes are
validated with the shared canonical little-endian FNV-1a-64 checksum. The
reader stages complete datasets, validates schema, bounds, header, and checksum,
then commits state; failed reads do not partially mutate the caller. Writers
publish through a temporary file followed by an atomic rename.

HDF5 headers and symbols are confined to `src/io/hdf5`. Public C and C++ SDK
headers expose neither HDF5 types nor HDF5 configuration. The adapter does not
implement MPI/HIP shard persistence, distributed HDF5, or a `format=hdf5` CLI
mode. When the optional dependency is unavailable, adapter operations return
`BLITZAR_STATUS_UNSUPPORTED` and the binary codec remains usable.

The HDF5 callback-data pointer used by the error-suppression scope is a registered
non-owning runtime boundary; it is not stored as simulation ownership.

## Evidence

`TST-P6-012` covers enabled round-trip, atomic publication, corruption and
truncation rejection, restart parity, repeated-write determinism, and the
unavailable-dependency fallback. `CHK-P0-038` validates this contract and
`CHK-P0-039` validates its deterministic fixtures. The installed package
consumer checks propagation of the capability state.
