# Decision 046: CLI Output Integration Boundary

Status: accepted
Issue: #645
Plan version: 1.0.29

## Context

The rewrite already has a validated binary snapshot codec and a deterministic
single-rank metadata lifecycle, but the configured CLI run did not consume its
output policy. The command executed the simulation and printed a console
summary only. Output ownership must be added without making the simulation
own filesystem state or duplicating the execution path.

## Decision

Materialize the CLI output boundary in apps/blitzar/BlitzarOutput:

- BlitzarRun remains the composition root. It configures and owns the
  Simulation, while BlitzarOutput owns MetadataRun and the contiguous snapshot
  IDs.
- BlitzarOutput::Prepare creates all output directories, reserves the ID
  buffer, and writes the initial manifest before the first simulation step.
  Preparation failures return a status in the output-prepare phase.
- The scheduler publishes step zero only when write_initial is true. After
  each successful step it publishes when the interval matches or when the
  completed step is the requested final step. A final step matching the
  interval is visited once, so it cannot be duplicated.
- Relative output directories continue to resolve from the configuration file
  directory. A periodic-only policy is valid because every_steps is itself a
  complete publication schedule.
- State capture and publication failures stop the run immediately and report
  output-state or output-publish with the returned status. No public C or C++
  ABI is expanded.
- The first CLI integration remains single-rank. The metadata lifecycle still
  creates the diagnostics directory, while diagnostic data remains owned by
  the separate conservation-metrics issue.

The integration is qualified by TST-P6-004, which exercises initial,
periodic-only, final-only, and initial/periodic/final schedules, decodes every
published snapshot, checks manifest order and file counts, and verifies
non-empty rerun rejection. The no-argument smoke path remains unchanged.

## Consequences

Configured CLI runs now produce the frozen manifest and binary state tree
through the same SDK-backed simulation path that no-output runs use. The
simulation and writer lifetimes are independent, output failures are visible
at a named phase, and no partial completed output is silently accepted.
Diagnostics CSV generation, restart, post-processing, distributed shards, and
HDF5 remain separate work.
