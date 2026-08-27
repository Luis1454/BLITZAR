# Decision 034: Final Repository Qualification

Status: accepted
Issue: #613
Plan version: 1.0.17

## Decision

The final clean-room closure is driven by the versioned contract in
`plan/final_audit.json`. It assigns an owner, category, and gate to every
tracked path, records the merged implementation commit and evidence reference
for RR-01 through RR-15, and names every supported CI lane.

`tools/audit/audit_final.py` produces an external audit directory containing the
complete file matrix, content scan, accepted/deferred finding register, lane
states, historical commit checks, and static quality-gate log. The tool can
run in local report mode with unreported remote lanes, or in strict CI mode
where every upstream lane must be successful.

## Consequences

- Every tracked file is accounted for without treating line count as an
  architectural violation.
- Existing architecture reviews and deferred capabilities remain explicit
  findings rather than being hidden by a clean status.
- The final report cannot claim multi-node MPI or physical GPU execution from
  a CPU-only or single-host environment.
- Generated audit artifacts remain outside the source tree and are published
  by CI rather than committed as build output.
