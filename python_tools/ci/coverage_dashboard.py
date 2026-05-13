#!/usr/bin/env python3
# @file python_tools/ci/coverage_dashboard.py
# @author Luis1454
# @project BLITZAR
# @brief Python quality and automation support for BLITZAR governance.

from __future__ import annotations

import html
import json
from dataclasses import dataclass
from datetime import UTC, datetime
from pathlib import Path


@dataclass(frozen=True)
# @brief Defines the coverage metrics type contract.
# @param None This contract does not take explicit parameters.
# @note Keep construction and side effects explicit for deterministic quality gates.
class CoverageMetrics:
    lines: float
    functions: float
    branches: float


# @brief Defines the coverage dashboard builder type contract.
# @param None This contract does not take explicit parameters.
# @note Keep construction and side effects explicit for deterministic quality gates.
class CoverageDashboardBuilder:
    # @brief Documents the color for operation contract.
    # @param pct Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
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

    # @brief Documents the build operation contract.
    # @param root Input value used by this contract.
    # @param metrics Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def build(self, root: Path, metrics: CoverageMetrics) -> Path:
        out = root / "coverage"
        out.mkdir(parents=True, exist_ok=True)
        updated = datetime.now(UTC).strftime("%Y-%m-%dT%H:%M:%SZ")
        self._write_badge(out, "lines", metrics.lines)
        self._write_badge(out, "functions", metrics.functions)
        self._write_badge(out, "branches", metrics.branches)
        self._write_widget(out, metrics, updated)
        summary = {
            "updated_utc": updated,
            "lines_pct": round(metrics.lines, 1),
            "functions_pct": round(metrics.functions, 1),
            "branches_pct": round(metrics.branches, 1),
        }
        (out / "summary.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")
        self._write_index(root, metrics, updated)
        return root / "index.html"

    # @brief Documents the write badge operation contract.
    # @param out Input value used by this contract.
    # @param name Input value used by this contract.
    # @param pct Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _write_badge(self, out: Path, name: str, pct: float) -> None:
        payload = {"schemaVersion": 1, "label": name, "message": f"{pct:.1f}%", "color": self.color_for(pct)}
        (out / f"{name}.json").write_text(json.dumps(payload), encoding="utf-8")

    # @brief Documents the svg color operation contract.
    # @param slug Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _svg_color(self, slug: str) -> str:
        mapping = {
            "brightgreen": "#2ea043",
            "green": "#3fb950",
            "yellow": "#d29922",
            "orange": "#fb8500",
            "red": "#cf222e",
        }
        return mapping.get(slug, "#6e7781")

    # @brief Documents the circle markup operation contract.
    # @param cx Input value used by this contract.
    # @param cy Input value used by this contract.
    # @param radius Input value used by this contract.
    # @param label Input value used by this contract.
    # @param pct Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _circle_markup(self, cx: int, cy: int, radius: int, label: str, pct: float) -> str:
        circumference = 2.0 * 3.141592653589793 * float(radius)
        dash = circumference * max(0.0, min(pct, 100.0)) / 100.0
        gap = circumference - dash
        color = self._svg_color(self.color_for(pct))
        safe_label = html.escape(label)
        return (
            f"<circle cx='{cx}' cy='{cy}' r='{radius}' fill='none' stroke='#d0d7de' stroke-width='14' />"
            f"<circle cx='{cx}' cy='{cy}' r='{radius}' fill='none' stroke='{color}' stroke-width='14' "
            f"stroke-linecap='round' stroke-dasharray='{dash:.1f} {gap:.1f}' "
            f"transform='rotate(-90 {cx} {cy})' />"
            f"<text x='{cx}' y='{cy + 4}' text-anchor='middle' class='metric-value'>{pct:.1f}%</text>"
            f"<text x='{cx}' y='{cy + 30}' text-anchor='middle' class='metric-label'>{safe_label}</text>"
        )

    # @brief Documents the write widget operation contract.
    # @param out Input value used by this contract.
    # @param metrics Input value used by this contract.
    # @param updated Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _write_widget(self, out: Path, metrics: CoverageMetrics, updated: str) -> None:
        circles = (
            self._circle_markup(135, 132, 58, "Lines", metrics.lines)
            + self._circle_markup(390, 132, 58, "Functions", metrics.functions)
            + self._circle_markup(645, 132, 58, "Branches", metrics.branches)
        )
        svg = (
            "<svg xmlns='http://www.w3.org/2000/svg' width='780' height='280' viewBox='0 0 780 280' role='img' "
            "aria-labelledby='title desc'>"
            "<title id='title'>BLITZAR integration coverage</title>"
            "<desc id='desc'>Lines, functions, and branches coverage percentages published by nightly-full.</desc>"
            "<style>"
            "text{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Helvetica,Arial,sans-serif;}"
            ".title{font-size:30px;font-weight:700;fill:#24292f;}"
            ".subtitle{font-size:16px;fill:#57606a;}"
            ".metric-value{font-size:28px;font-weight:700;fill:#24292f;}"
            ".metric-label{font-size:16px;fill:#57606a;}"
            ".updated{font-size:14px;fill:#57606a;}"
            "</style>"
            "<rect width='780' height='280' rx='18' fill='#ffffff' stroke='#d0d7de'/>"
            "<text x='32' y='52' class='title'>Coverage Control</text>"
            "<text x='32' y='78' class='subtitle'>Nightly integration coverage steering widget</text>"
            f"{circles}"
            f"<text x='32' y='250' class='updated'>Updated {html.escape(updated)}</text>"
            "</svg>"
        )
        (out / "widget.svg").write_text(svg, encoding="utf-8")

    # @brief Documents the write index operation contract.
    # @param root Input value used by this contract.
    # @param metrics Input value used by this contract.
    # @param updated Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _write_index(self, root: Path, metrics: CoverageMetrics, updated: str) -> None:
        content = (
            "<!doctype html><html><head><meta charset='utf-8'>"
            "<meta name='viewport' content='width=device-width, initial-scale=1'>"
            "<title>Coverage Dashboard</title></head><body>"
            "<h1>Integration Coverage</h1>"
            f"<p>Updated: {updated}</p>"
            "<p><img src='coverage/widget.svg' alt='Coverage control widget' style='max-width:100%;height:auto'></p>"
            "<ul>"
            f"<li>Lines: {metrics.lines:.1f}%</li>"
            f"<li>Functions: {metrics.functions:.1f}%</li>"
            f"<li>Branches: {metrics.branches:.1f}%</li>"
            "</ul>"
            "<p>JSON endpoints: "
            "<a href='coverage/lines.json'>lines</a>, "
            "<a href='coverage/functions.json'>functions</a>, "
            "<a href='coverage/branches.json'>branches</a>, "
            "<a href='coverage/summary.json'>summary</a>, "
            "<a href='coverage/widget.svg'>widget</a>"
            "</p></body></html>"
        )
        (root / "index.html").write_text(content, encoding="utf-8")
