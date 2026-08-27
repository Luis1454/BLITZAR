# Decision 049: CLI Production Run Summary

Status: accepted
Issue: #646
Plan version: 1.0.33

## Context

The configured CLI previously printed first and last particle samples as its
normal result. Those values are implementation-oriented smoke diagnostics and
do not describe whether a production run completed, what it produced, or how
it failed. The CLI needs a stable machine-readable boundary without changing
the public SDK ABI.

## Decision

The configured CLI emits exactly one one-line JSON summary to stdout after a
successful run. The field order is fixed and the summary contains:

- `schema_version`, `status`, `requested_steps`, and `completed_steps`;
- `particle_count`, `solver`, `snapshot_count`, and `diagnostics_count`;
- `output_path`, encoded as a portable generic path or an empty string when
  output is disabled.

The summary uses the classic locale and no wall-clock or sample values. The
diagnostics count is the number of records actually published, not the number
requested by configuration; it is zero until the diagnostics writer is
implemented.

Failures emit one one-line JSON object to stderr containing the schema version, status,
phase, exit code, and stable status message. Exit codes are fixed as follows:

- `0`: successful run;
- `2`: command-line usage error;
- `3`: configuration or input construction failure;
- `4`: runtime or simulation failure;
- `5`: output preparation, publication, or summary failure.

The normal smoke command remains a separate context check. No public ABI,
snapshot wire format, solver execution path, or diagnostic file format is
changed by this decision.

## Evidence

`TST-P6-006` asserts the exact stdout and stderr records, output counters,
portable output path, exit-code mapping, and byte-for-byte equality of repeated
deterministic no-output runs. `TST-P6-007` asserts the usage error emitted by
the executable entry point.
