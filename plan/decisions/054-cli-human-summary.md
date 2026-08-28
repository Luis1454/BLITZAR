# Decision 054: Human-Readable CLI Results

Status: accepted  
Issue: #670  
Plan version: 1.0.38

## Context

Decision 049 established a deterministic one-line JSON summary for configured
CLI runs. That format is suitable for scripts and CI, but it is not a useful
default presentation for a person running a simulation from a shell. The
post-processing command uses the same summary boundary and must not create a
second reporting vocabulary.

## Decision

The CLI supports two explicit result formats: `human` and `json`.

This decision supersedes Decision 049 only for the default presentation; its
JSON schema and machine-readable field order remain the automation contract.

- The default format is `human` for configured runs and post-processing.
- `--format human` selects the labeled shell presentation explicitly.
- `--format json` selects the existing one-line machine-readable schema.
- The option is accepted before or after `--config` or `--post-process`.
- Human output contains stable labels for status, solver, completed/requested
  steps, particle count, snapshot count, diagnostics count, and output path.
- Human failures contain phase, status, message, and exit code on stderr.
- JSON success and failure records keep their schema version, field order,
  locale, stream, and exit-code behavior.
- Human output contains no timestamps, samples, colors, or ANSI control
  sequences.

The public C/C++ SDK ABI, snapshot wire format, diagnostics file format,
simulation execution path, and output directory layout are unchanged.

## Compatibility

Automation must pass `--format json` rather than depending on the human
default. Internal test calls select the format through `BlitzarStreams`, so the
format remains explicit without adding more than four callable parameters.

## Evidence

`TST-P6-006` covers deterministic human and JSON configured-run summaries,
failure stream separation, counters, output paths, and repeated JSON output.
`TST-P6-007` covers explicit JSON usage errors and invalid-format rejection.
`TST-P6-010` covers human and JSON post-processing summaries while retaining
online/post-processing diagnostic parity.
