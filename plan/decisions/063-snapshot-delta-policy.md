# Decision 063: Snapshot Delta Policy

- Status: active
- Scope: clean-room issue #675
- Plan version: 1.0.47

## Decision

The existing binary snapshot remains the canonical output and restart format.
The delta codec is an internal candidate for snapshot and transport payloads;
it is not selected by the CLI, the C ABI, or the C++ SDK.

`SnapshotDelta` serializes the existing canonical SoA payload in the frozen
field order and applies an XOR against a base payload followed by bounded runs.
The high bit of each control byte identifies a zero run; the lower seven bits
encode the run length minus one. Literal runs store their XOR bytes after the
control byte. The stream header is 24 bytes and records the raw payload size
and its canonical FNV-1a-64 checksum. The maximum encoded allocation is
`24 + 2 * raw_payload_bytes`.

The caller owns all codec buffers. Encode and decode therefore have no
steady-state allocation and can be made reentrant by giving each operation a
separate `SnapshotDeltaBuffer`. Decode validates the complete stream and
checksum before unpacking into the destination; corruption and truncation do
not partially mutate caller state.

Delta streams use a keyframe every eight frames. A step index resolves to the
nearest keyframe and replays no more than seven deltas. Until a persistent
indexed stream is implemented and qualified, the binary snapshot is the
direct random-access restart fallback. The codec measures in-memory payload
staging; filesystem latency is deliberately excluded from this candidate
benchmark.

No numerical representation, public ABI, default output path, or HDF5 policy
changes in this decision.

## Evidence

`TST-P6-013` proves exact round-trip and indexed replay, deterministic bytes,
checksum validation, transactional rejection, reuse with non-contiguous
transport IDs, and a smaller candidate stream. It reports reference versus
delta write/read latency and logical workspace bytes. `CHK-P0-040` validates
this contract and `CHK-P0-041` validates its evidence fixtures.
