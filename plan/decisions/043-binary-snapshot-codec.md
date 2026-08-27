# Decision 043: Binary Snapshot Codec

Status: accepted
Issue: #642
Plan version: 1.0.27

## Context

The versioned `SnapshotFrameView` contract defines the logical frame, but it
does not define how a frame is materialized on disk. The first persistence
implementation must be deterministic, portable across supported hosts, and
safe against corrupt or oversized files without mutating caller storage during
validation.

## Decision

Materialize a single-rank binary codec under `src/io/snapshot` with these wire
rules:

- Every integer and IEEE-754 binary64 scalar is encoded explicitly in
  little-endian order; C++ object layout is never written directly.
- The header order is magic, version, scalar width, particle count, step, time,
  rank count, rank index, endianness, distribution, and ID policy.
- The payload order is IDs, position X/Y/Z, velocity X/Y/Z, and mass.
- The header is 43 bytes, each particle contributes 64 payload bytes, and the
  file ends with an 8-byte FNV-1a-64 checksum over the payload only.
- The reader bounds the file to the frozen particle maximum, validates header,
  exact size, payload values, checksum, and EOF before decoding into caller
  storage. The destination header and spans remain unchanged on rejection.
- The reader owns one prepared bounded byte buffer and does not grow it during
  `Read`; the writer emits directly to the destination stream.

The codec is single-rank only. MPI shards, HDF5, the deterministic run
manifest, atomic publication, and public persistence API remain outside this
issue and require their own contracts and evidence.

## Consequences

The binary representation is stable and byte-for-byte reproducible for the
same frame. Compatibility failures are classified before payload mutation, and
future persistence adapters must consume the logical frame contract instead of
depending on C++ struct layout.
