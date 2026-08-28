# Decision 051: Snapshot Restart Boundary Hardening

Status: **accepted**
Issue: **#666**
Plan version: **1.0.35**

## Context

The deterministic restart contract validates the snapshot checksum and finite
payload values, but a checksum-valid negative mass could cross the reader
boundary before the later particle staging check. The existing restart test
also called the internal `RunConfig` function directly and used a fixed
temporary directory, leaving the production argument parser and concurrent
test isolation unqualified.

## Decision

Snapshot masses are valid only when finite and nonnegative. The frame contract
and the reader's validation pass enforce this rule before payload decoding,
header publication, or restart-state commit. A rejected payload leaves both
the destination state and restart time unchanged.

The restart qualification keeps two explicit layers. `TST-P6-008` validates
the internal restart transaction and malformed payload boundaries. `TST-P6-009`
launches the production `blitzar_cli --config` entrypoint and compares an
uninterrupted run with a split-and-restarted run. Both tests acquire a bounded,
race-safe temporary directory and fail when cleanup reports an error.

The process test's `main` argument array is registered as a borrowed execution
boundary in the pointer ownership contract. It is used only to receive the
CLI executable path; no ownership is transferred across that boundary.

## Boundaries

This decision remains limited to single-rank CPU restart behavior. It does not
claim MPI or sharded persistence, HIP or CUDA restart state, HDF5, or solver
coverage beyond the Direct path used by the deterministic fixture.

## Evidence

`TST-P6-008` covers checksum-valid negative mass rejection, transactional state
preservation, compatibility rejection, and malformed snapshot handling.
`TST-P6-009` covers the real executable argument boundary, isolated process
artifacts, and byte-identical uninterrupted versus restarted output.
