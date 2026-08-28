# Decision 053: MPI and HIP Output Qualification Boundary

- Status: accepted
- Issue: #649
- Plan version: 1.0.37

## Context

The simulation facade can gather a distributed state for MPI qualification. That
operation is useful for internal parity tests, but it must not be reached
implicitly by the CLI persistence path. Snapshot serialization also has to stay
independent from both MPI and HIP implementation headers. Finally, timing the
physics loop and timing output publication in one interval would make output
overhead impossible to interpret.

## Decision

When configured output is enabled, the CLI performs an internal
`MpiContext` topology preflight before it builds the initial simulation state.
Single-rank execution continues through the existing output pipeline. A context
with more than one rank returns `BLITZAR_STATUS_UNSUPPORTED` in the
`output-topology` phase before state capture, output preparation, or any
distributed gather can occur. The internal `Sim::GetState` gather contract is
unchanged for MPI tests that explicitly request global state.

The single-rank MPI qualification invokes the same production CLI once directly
and once through `mpiexec -np 1`, then compares the manifest and every emitted
snapshot byte-for-byte. The two-rank case must fail with the output-topology
status and must not create a run output directory. No shard or hidden full-gather
format is introduced by this issue.

`RunSteps` measures `Simulation::step` separately from each configured output
checkpoint. The output interval includes host state capture and snapshot or
diagnostic publication. Timing is exposed only to the internal qualification
fixture through `BlitzarRunTiming`; it is not persisted in manifests, snapshots,
or the public summary.

The snapshot writer and metadata modules remain free of MPI and HIP headers. The
existing output scheduling test exercises the checkpoint counter and the timing
separation in CPU-only, HIP-compiled fallback, and HIP device builds when those
capabilities are available.

## Consequences

`TST-P6-004` qualifies checkpoint scheduling and separate timing, while
`TST-P6-011` qualifies direct versus single-rank MPI output and the explicit
multi-rank rejection. MPI output evidence is limited to one host. RDMA, real
multi-node persistence, HIP device availability, and sharded output remain
unverified or deferred and require their own contracts.
