#!/usr/bin/env python3
from __future__ import annotations

import json
from dataclasses import dataclass
from datetime import UTC, datetime
from pathlib import Path


@dataclass(frozen=True)
class CoverageMetrics:
    lines: float
    functions: float
    branches: float


class CoverageDashboardBuilder:
    def color_for(self, pct: float) -> str:
        if pct >= 90.0:
            return "brightgreen"
        if pct >= 75.0:
            return "green"
        if pct >= 60.0:
            return "yellow"
        if pct >= 40.0:
            return "orange"
        return "red"

    def build(self, root: Path, metrics: CoverageMetrics) -> Path:
        out = root / "coverage"
        out.mkdir(parents=True, exist_ok=True)
        updated = datetime.now(UTC).strftime("%Y-%m-%dT%H:%M:%SZ")
        self._write_badge(out, "lines", metrics.lines)
        self._write_badge(out, "functions", metrics.functions)
        self._write_badge(out, "branches", metrics.branches)
        summary = {
            "updated_utc": updated,
            "lines_pct": round(metrics.lines, 1),
            "functions_pct": round(metrics.functions, 1),
            "branches_pct": round(metrics.branches, 1),
        }
        (out / "summary.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")
        self._write_index(root, metrics, updated)
        return root / "index.html"

    def _write_badge(self, out: Path, name: str, pct: float) -> None:
        payload = {"schemaVersion": 1, "label": name, "message": f"{pct:.1f}%", "color": self.color_for(pct)}
        (out / f"{name}.json").write_text(json.dumps(payload), encoding="utf-8")

    def _write_index(self, root: Path, metrics: CoverageMetrics, updated: str) -> None:
        content = (
            "<!doctype html><html><head><meta charset='utf-8'>"
            "<meta name='viewport' content='width=device-width, initial-scale=1'>"
            "<title>Coverage Dashboard</title></head><body>"
            "<h1>Integration Coverage</h1>"
            f"<p>Updated: {updated}</p>"
            "<ul>"
            f"<li>Lines: {metrics.lines:.1f}%</li>"
            f"<li>Functions: {metrics.functions:.1f}%</li>"
            f"<li>Branches: {metrics.branches:.1f}%</li>"
            "</ul>"
            "<p>JSON endpoints: "
            "<a href='coverage/lines.json'>lines</a>, "
            "<a href='coverage/functions.json'>functions</a>, "
            "<a href='coverage/branches.json'>branches</a>, "
            "<a href='coverage/summary.json'>summary</a>"
            "</p></body></html>"
        )
        (root / "index.html").write_text(content, encoding="utf-8")

