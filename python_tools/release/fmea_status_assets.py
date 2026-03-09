from __future__ import annotations

from collections.abc import Mapping, Sequence


def render_fmea_status_readme(snapshot: Mapping[str, object]) -> str:
    summary = snapshot.get("summary")
    if not isinstance(summary, dict):
        summary = {}
    high_medium = snapshot.get("high_medium")
    if not isinstance(high_medium, Sequence):
        high_medium = ()
    lines = [
        "# FMEA Risk Status Snapshot",
        "",
        f"- Open: `{summary.get('open', 0)}`",
        f"- In Progress: `{summary.get('in_progress', 0)}`",
        f"- Closed: `{summary.get('closed', 0)}`",
        f"- High/Medium residual risks: `{len(high_medium)}`",
        "",
        "See `fmea_status_snapshot.json` for machine-readable details.",
    ]
    return "\n".join(lines) + "\n"

