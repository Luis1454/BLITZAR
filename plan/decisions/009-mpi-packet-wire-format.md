# Decision 009: Canonical MPI Packet Wire Format

Status: accepted
Plan version: 1.0.6

## Decision

MPI transport never sends the in-memory representation of `ParticlePacket`.
Every packet is encoded into a fixed 64-byte wire record:

- bytes 0-7: unsigned 64-bit particle ID;
- bytes 8-63: seven IEEE-754 binary64 scalar values in packet order;
- all integer and scalar bit patterns are serialized little-endian;
- compiler padding and host object layout are not part of the protocol.

`ParticleWireCodec` owns this representation and is used by migration,
gather, and ghost exchanges. The current `Scalar` contract is binary64; a
different width or a non-IEEE scalar is rejected before an MPI data
collective. Mixed-precision and byte-order conversion outside this codec are
not implicit policy.

MPI count and displacement arrays remain `int` because that is the contract of
the selected MPI collectives. Packet transfers are therefore processed in
rounds. Each round computes a per-peer packet limit so the complete temporary
wire buffer and every byte count/displacement stay at or below `INT_MAX`.
Point-to-point ghost transfers use the same limit and post one request per
wire chunk. Size multiplication and allocation failures are validated and
synchronized before data operations.

## Consequences

- Packet bytes are independent of compiler padding, host endianness, and
  `ParticlePacket` layout.
- A packet count that cannot be represented safely is rejected before the
  corresponding collective or request batch.
- Large transfers require temporary wire buffers and multiple MPI rounds, but
  the public packet-unit spans remain unchanged.
- The wire fixture in `tests/mpi/MpiTest.cpp` locks the scalar width and little-endian
  byte order contract.
