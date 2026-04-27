# File: python_tools/ci/performance_benchmark.py
# Purpose: Python quality and automation support for BLITZAR governance.

from __future__ import annotations

import json
import shutil
import subprocess
from collections.abc import Callable, Mapping
from datetime import UTC, datetime
from pathlib import Path


class PerformanceBenchmarkCampaign:
    def __init__(self, runner: Callable[[list[str]], str] | None = None) -> None:
        self._runner = runner

    def load_profile(self, root: Path, profile: str) -> dict[str, object]:
        payload = json.loads((root / "docs/quality/manifest/performance_campaign.json").read_text(encoding="utf-8"))
        profiles = payload.get("profiles")
        if not isinstance(profiles, dict):
            raise PerformanceBenchmarkError("performance campaign profiles payload is unavailable")
        profile_payload = profiles.get(profile)
        if not isinstance(profile_payload, dict):
            raise PerformanceBenchmarkError(f"unknown performance benchmark profile: {profile}")
        return profile_payload

    def run(self, root: Path, dist_dir: Path, profile: str, tool_path: Path) -> tuple[Path, dict[str, object]]:
        profile_payload = self.load_profile(root.resolve(), profile)
        runs = self._collect_runs(tool_path.resolve(), profile_payload.get("runs"))
        report = self._build_report(profile, runs)
        archive = self._archive(dist_dir.resolve(), report)
        return archive, report

    def _collect_runs(self, tool_path: Path, raw_runs: object) -> list[dict[str, object]]:
        if not isinstance(raw_runs, list):
            raise PerformanceBenchmarkError("performance benchmark runs must be a list")
        rows: list[dict[str, object]] = []
        for entry in raw_runs:
            if not isinstance(entry, dict):
                raise PerformanceBenchmarkError("performance benchmark run entries must be objects")
            command = [
                str(tool_path),
                "--workload",
                require_string(entry, "workload"),
                "--solver",
                require_string(entry, "solver"),
                "--integrator",
                require_string(entry, "integrator"),
                "--dt",
                str(require_float(entry, "dt")),
                "--particle-count",
                str(require_int(entry, "particle_count")),
                "--steps",
                str(require_int(entry, "steps")),
                "--seed",
                str(require_int(entry, "seed")),
            ]
            measurement = parse_measurement(self._run_command(command))
            measurement.update(
                {
                    "id": require_string(entry, "id"),
                    "checks": require_mapping(entry.get("checks")),
                    "baseline": require_mapping(entry.get("baseline")),
                    "regressions": require_mapping(entry.get("regressions")),
                }
            )
            rows.append(measurement)
        return rows

    def _run_command(self, command: list[str]) -> str:
        if self._runner is not None:
            return self._runner(command)
        completed = subprocess.run(command, capture_output=True, text=True, check=False)
        if completed.returncode != 0:
            raise PerformanceBenchmarkError(completed.stderr.strip() or completed.stdout.strip() or "benchmark tool failed")
        return completed.stdout

    def _build_report(self, profile: str, runs: list[dict[str, object]]) -> dict[str, object]:
        failures: list[str] = []
        run_rows = [self._evaluate_run(run, failures) for run in runs]
        return {
            "format_version": "1.0",
            "generated_at_utc": datetime.now(UTC).isoformat(timespec="seconds"),
            "profile": profile,
            "status": "failed" if failures else "passed",
            "runs": run_rows,
            "failures": failures,
        }

    def _evaluate_run(self, run: dict[str, object], failures: list[str]) -> dict[str, object]:
        check_rows = []
        for metric_name, threshold in sorted(require_mapping(run.get("checks")).items()):
            measured_name = metric_name[4:] if metric_name.startswith("min_") else (metric_name[4:] if metric_name.startswith("max_") else metric_name)
            value = require_float(run, measured_name)
            passed = (value >= threshold) if metric_name.startswith("min_") else (value <= threshold)
            check_rows.append({"metric": metric_name, "value": value, "threshold": threshold, "passed": passed})
            if not passed:
                failures.append(f"{run['id']}: {metric_name}={value:.6f} violates threshold {threshold:.6f}")

        regression_rows = []
        baselines = require_mapping(run.get("baseline"))
        limits = require_mapping(run.get("regressions"))
        for metric_name, limit_pct in sorted(limits.items()):
            if metric_name not in baselines:
                raise PerformanceBenchmarkError(f"{run['id']}: missing baseline for regression metric {metric_name}")
            value = require_float(run, metric_name)
            baseline_value = baselines[metric_name]
            regression_pct = max(0.0, (baseline_value - value) / max(abs(baseline_value), 1e-6) * 100.0)
            passed = regression_pct <= limit_pct
            regression_rows.append(
                {
                    "metric": metric_name,
                    "value": value,
                    "baseline": baseline_value,
                    "regression_pct": regression_pct,
                    "limit_pct": limit_pct,
                    "passed": passed,
                }
            )
            if not passed:
                failures.append(
                    f"{run['id']}: {metric_name} regressed by {regression_pct:.2f}% against baseline {baseline_value:.6f}"
                )

        return {
            "id": run["id"],
            "workload": run["workload"],
            "solver": run["solver"],
            "integrator": run["integrator"],
            "dt": run["dt"],
            "particle_count": run["particle_count"],
            "steps": run["steps"],
            "seed": run["seed"],
            "metrics": {
                "wall_seconds": require_float(run, "wall_seconds"),
                "steps_per_second": require_float(run, "steps_per_second"),
                "particles_per_second": require_float(run, "particles_per_second"),
                "average_step_ms": require_float(run, "average_step_ms"),
            },
            "checks": check_rows,
            "regressions": regression_rows,
        }

    def _archive(self, dist_dir: Path, report: dict[str, object]) -> Path:
        if dist_dir.exists():
            shutil.rmtree(dist_dir)
        dist_dir.mkdir(parents=True, exist_ok=True)
        (dist_dir / "performance_benchmark_report.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
        (dist_dir / "README.md").write_text(render_performance_readme(report), encoding="utf-8")
        archive_base = dist_dir / "BLITZAR-performance-benchmark"
        return Path(shutil.make_archive(str(archive_base), "zip", root_dir=dist_dir))


class PerformanceBenchmarkError(RuntimeError):
    pass


def parse_measurement(stdout: str) -> dict[str, object]:
    result: dict[str, object] = {}
    for raw_line in stdout.splitlines():
        line = raw_line.strip()
        if not line or "=" not in line:
            continue
        name, raw_value = line.split("=", 1)
        try:
            result[name] = float(raw_value) if any(char in raw_value for char in ".eE") else int(raw_value)
        except ValueError:
            result[name] = raw_value
    return result


def render_performance_readme(report: Mapping[str, object]) -> str:
    raw_runs = report.get("runs")
    raw_failures = report.get("failures")
    run_rows: list[object] = raw_runs if isinstance(raw_runs, list) else []
    failures: list[object] = raw_failures if isinstance(raw_failures, list) else []
    lines = [
        "# Performance Benchmark Report",
        "",
        f"- Profile: `{report.get('profile', '')}`",
        f"- Status: `{report.get('status', '')}`",
        f"- Runs: `{len(run_rows)}`",
        f"- Failures: `{len(failures)}`",
        "",
        "| Run | Steps/s | Avg step ms |",
        "| --- | ---: | ---: |",
    ]
    for run in run_rows:
        if not isinstance(run, dict):
            continue
        metrics = run.get("metrics")
        if not isinstance(metrics, dict):
            continue
        lines.append(
            f"| `{run.get('id', '')}` | {float(metrics.get('steps_per_second', 0.0)):.2f} | {float(metrics.get('average_step_ms', 0.0)):.3f} |"
        )
    if failures:
        lines.extend(["", "## Failures", ""])
        for failure in failures:
            lines.append(f"- {failure}")
    return "\n".join(lines) + "\n"


def require_mapping(raw: object) -> dict[str, float]:
    if raw is None:
        return {}
    if not isinstance(raw, dict):
        raise PerformanceBenchmarkError("benchmark threshold payload must be an object")
    return {str(name): float(value) for name, value in raw.items()}


def require_string(row: dict[str, object], field: str) -> str:
    value = row.get(field)
    if not isinstance(value, str) or not value.strip():
        raise PerformanceBenchmarkError(f"missing required field: {field}")
    return value.strip()


def require_float(row: dict[str, object], field: str) -> float:
    value = row.get(field)
    if isinstance(value, (int, float)):
        return float(value)
    raise PerformanceBenchmarkError(f"missing numeric field: {field}")


def require_int(row: dict[str, object], field: str) -> int:
    value = row.get(field)
    if isinstance(value, int):
        return value
    raise PerformanceBenchmarkError(f"missing integer field: {field}")
