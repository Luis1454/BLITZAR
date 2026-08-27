# Decision 042: Versioned Snapshot Frame

Status: accepted
Issue: #641
Plan version: 1.0.26

## Context

The output contract defines a binary snapshot format, but the repository had
only a minimal `SnapshotHeader` hook. A writer must receive a logical frame
without depending on C++ object layout, owning simulation memory, or importing
MPI or HIP types.

## Decision

`src/core/CoreSnapshot.hpp` owns the value-only frame contract:

- `SnapshotHeader` uses fixed-width protocol fields for the magic, version,
  scalar width, particle count, step, time, rank metadata, distribution, and
  identity policy.
- `SnapshotHeaderFieldOrder` and `SnapshotPayloadOrder` define the stable
  header and SoA field order. These orders are metadata for an explicit codec;
  no C++ object bytes are part of the format.
- `SnapshotPayloadView` contains only const spans for stable global IDs,
  position, velocity, and mass. Single-rank frames require IDs in
  `[0, particle_count)` and finite scalar values.
- `SnapshotFrameView` combines the header and payload and validates all span
  lengths before a writer can consume it.
- Version and scalar-width mismatches return `BLITZAR_STATUS_UNSUPPORTED`.
  Malformed metadata, mismatched spans, non-finite values, and invalid IDs
  return `BLITZAR_STATUS_INVALID_ARGUMENT`.
- Single-rank frames are the only supported distribution in version 1.
  Sharded frames have an explicit distribution and stable-global-ID policy,
  but return `BLITZAR_STATUS_UNSUPPORTED` until the shard manifest and codec
  are implemented.

The public C and C++ SDK remains unchanged. Disk access, serialization,
checksums, atomic publication, MPI output, and HDF5 remain outside this issue.

## Consequences

The future IO writer receives a non-owning, immutable, validated view with a
deterministic field order. Header validation is independent from serialization
and rejects unsupported protocol variants before any destination state can be
modified. The existing `src/io` deferred root remains the next persistence
boundary rather than becoming a mixed contract and implementation directory.
