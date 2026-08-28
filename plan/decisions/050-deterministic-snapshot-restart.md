# Decision 050: Deterministic Snapshot Restart

Status: **accepted**
Issue: **#647**
Plan version: **1.0.34**

## Context

The single-rank snapshot codec and run manifest provide a versioned state
boundary, but the CLI previously always regenerated the deterministic initial
state. A restart must continue from a selected persisted state without
silently accepting a different solver, numerical configuration, scalar format,
or particle identity set.

## Decision

The CLI accepts the optional directive
`restart(directory="...", step=K)`. The directory contains the source
`manifest.json` and `states/state-%08d.bin`; `K` is an explicit absolute
simulation step. The configured `run(steps=N)` remains an absolute final step,
so a restart executes steps `K + 1` through `N` and requires `K < N`.

The source manifest is authoritative for static compatibility metadata. The
selected snapshot header and payload are authoritative for the dynamic state.
Before mutating the caller-visible state, the loader validates the product and
plan versions, solver, integrator, timestep, gravity, units, Barnes-Hut
parameters, generation seed/determinism, particle count, distribution, scalar
width, IDs, checksum, step, and time. The payload is decoded into a candidate
state and committed only after every validation succeeds.

Fresh runs keep step zero semantics. Restart runs may publish the selected
initial state at `K`; later output scheduling and summary counters use absolute
steps. Frame times after the selected state are computed from the absolute step
to preserve byte-identical split-run output.

## Boundaries

This decision covers the deterministic single-rank binary codec only. HDF5
restart, distributed/sharded restart, MPI migration state, and GPU-specific
restart state remain unsupported. The public C ABI and C++ facade are unchanged;
restart is an internal CLI configuration contract.

## Evidence

`TST-P6-008` covers uninterrupted versus split-run byte parity, explicit
compatibility rejection, checksum and truncation rejection, and transactional
failure without partial state mutation.
