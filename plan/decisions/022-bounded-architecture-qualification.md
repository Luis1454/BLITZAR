# Decision 022: Bounded Architecture Qualification

Status: accepted
Plan version: 1.0.6

## Decision

The repository uses `tools/architecture/architecture_report.py` as a deterministic,
source-derived architecture report. It measures callable body size and count,
branch points, lexical allocation sites, internal include dependencies, and
callable parameter counts after applying only the documented ABI V1/V2
exceptions.

The report is a responsibility signal, not a mechanical file-splitting rule.
Physical line count is emitted as informational data and never creates a
review signal. A signal above its threshold requires an entry in
`plan/architecture_reviews.json` explaining why the responsibility boundary
is retained or what bounded follow-up owns the change.

The quality manifest freezes the threshold contract. The report and its fixture
tests run in the plan workflow, while runtime evidence remains the output of
the CPU, MPI, package, sanitizer, and available GPU qualification lanes.

## Consequences

- High-complexity files are visible without pretending that line count proves
  a design defect.
- Review decisions are explicit, versioned, and checked for stale or missing
  entries.
- The four-parameter rule remains enforced by `tools/gates/argument_gate.py`; the
  architecture report does not create undocumented exemptions.
- ABI V1 compatibility is qualified by a C consumer, and versioned ABI V2
  descriptors are qualified by the same public-header-only boundary.
