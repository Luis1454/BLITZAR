from __future__ import annotations

from collections.abc import Mapping, Sequence


def render_numerical_validation_readme(report: Mapping[str, object]) -> str:
    run_rows = report.get("runs")
    if not isinstance(run_rows, Sequence):
        run_rows = ()
    comparison_rows = report.get("comparisons")
    if not isinstance(comparison_rows, Sequence):
        comparison_rows = ()
    failures = report.get("failures")
    if not isinstance(failures, Sequence):
        failures = ()
    lines = [
        "# Numerical Validation Report",
        "",
        f"- Profile: `{report.get('profile', '')}`",
        f"- Status: `{report.get('status', '')}`",
        f"- Runs: `{len(run_rows)}`",
        f"- Comparisons: `{len(comparison_rows)}`",
        f"- Failures: `{len(failures)}`",
        "",
        "See `numerical_validation_report.json` for machine-readable details.",
    ]
    if failures:
        lines.extend(["", "## Failures", ""])
        for failure in failures:
            lines.append(f"- {failure}")
    return "\n".join(lines) + "\n"
