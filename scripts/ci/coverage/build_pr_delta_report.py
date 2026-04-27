#!/usr/bin/env python3
# File: scripts/ci/coverage/build_pr_delta_report.py
# Purpose: Automation script for BLITZAR build, release, or operations tasks.

from __future__ import annotations

import argparse
import csv
import json
import re
import urllib.request
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Build per-PR coverage delta summary and residual gaps table.")
    parser.add_argument("--summary", required=True, help="Path to gcovr text summary file.")
    parser.add_argument("--files-csv", required=True, help="Path to gcovr per-file CSV report.")
    parser.add_argument("--repo", required=True, help="GitHub repo in owner/name format.")
    parser.add_argument("--baseline-ref", default="coverage-data", help="Git ref that stores coverage badge JSON payloads.")
    parser.add_argument("--output", required=True, help="Output markdown file.")
    return parser.parse_args()


def parse_summary(summary_path: Path) -> dict[str, float]:
    metrics: dict[str, float] = {}
    pattern = re.compile(r"^(lines|functions|branches):\s+([0-9]+(?:\.[0-9]+)?)%")
    for line in summary_path.read_text(encoding="utf-8", errors="replace").splitlines():
        match = pattern.match(line.strip())
        if not match:
            continue
        metrics[match.group(1)] = float(match.group(2))
    required = {"lines", "functions", "branches"}
    if metrics.keys() < required:
        missing = sorted(required.difference(metrics.keys()))
        raise ValueError(f"summary missing metrics: {', '.join(missing)}")
    return metrics


def fetch_baseline_metric(repo: str, ref: str, name: str) -> float:
    url = f"https://raw.githubusercontent.com/{repo}/{ref}/coverage/{name}.json"
    with urllib.request.urlopen(url, timeout=20) as response:
        payload = json.loads(response.read().decode("utf-8"))
    message = str(payload.get("message", "")).strip().rstrip("%")
    try:
        return float(message)
    except ValueError as exc:
        raise ValueError(f"invalid baseline metric in {name}.json: {message!r}") from exc


def parse_residual_files(csv_path: Path, top_n: int = 10) -> list[tuple[str, int, int, float]]:
    rows: list[tuple[str, int, int, float]] = []
    with csv_path.open("r", encoding="utf-8", newline="") as handle:
        reader = csv.DictReader(handle)
        for row in reader:
            file_name = str(row.get("filename", "")).strip()
            if not file_name:
                continue
            branch_total = int(float(row.get("branch_total", "0") or "0"))
            branch_covered = int(float(row.get("branch_covered", "0") or "0"))
            branch_missed = max(0, branch_total - branch_covered)
            if branch_total <= 0 or branch_missed <= 0:
                continue
            pct = (branch_covered * 100.0) / branch_total
            rows.append((file_name, branch_missed, branch_total, pct))
    rows.sort(key=lambda item: (-item[1], item[0]))
    return rows[:top_n]


def signed_delta(current: float, baseline: float) -> str:
    delta = current - baseline
    return f"{delta:+.2f}"


def build_markdown(
    current: dict[str, float],
    baseline: dict[str, float],
    residual: list[tuple[str, int, int, float]],
) -> str:
    lines: list[str] = []
    lines.append("## Coverage Delta (PR vs baseline)")
    lines.append("")
    lines.append("| Metric | Baseline (%) | PR (%) | Delta (pp) |")
    lines.append("|---|---:|---:|---:|")
    lines.append(f"| Lines | {baseline['lines']:.2f} | {current['lines']:.2f} | {signed_delta(current['lines'], baseline['lines'])} |")
    lines.append(f"| Functions | {baseline['functions']:.2f} | {current['functions']:.2f} | {signed_delta(current['functions'], baseline['functions'])} |")
    lines.append(f"| Branches | {baseline['branches']:.2f} | {current['branches']:.2f} | {signed_delta(current['branches'], baseline['branches'])} |")
    lines.append("")
    lines.append("## Residual Missed-Branch Files")
    lines.append("")
    if not residual:
        lines.append("No residual missed-branch files were found in the current report.")
    else:
        lines.append("| File | Missed branches | Total branches | Branch coverage (%) |")
        lines.append("|---|---:|---:|---:|")
        for file_name, missed, total, pct in residual:
            lines.append(f"| {file_name} | {missed} | {total} | {pct:.2f} |")
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    args = parse_args()
    summary_path = Path(args.summary)
    csv_path = Path(args.files_csv)
    output_path = Path(args.output)

    current = parse_summary(summary_path)
    baseline = {
        "lines": fetch_baseline_metric(args.repo, args.baseline_ref, "lines"),
        "functions": fetch_baseline_metric(args.repo, args.baseline_ref, "functions"),
        "branches": fetch_baseline_metric(args.repo, args.baseline_ref, "branches"),
    }
    residual = parse_residual_files(csv_path)

    markdown = build_markdown(current, baseline, residual)
    output_path.write_text(markdown, encoding="utf-8")
    print(markdown)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
