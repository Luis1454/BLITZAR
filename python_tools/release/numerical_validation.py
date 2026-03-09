from __future__ import annotations

import json
import shutil
import subprocess
from datetime import UTC, datetime
from pathlib import Path

from python_tools.release.numerical_validation_assets import render_numerical_validation_readme
from python_tools.release.numerical_validation_support import (
    NumericalValidationError,
    as_mapping,
    comparison_metric,
    parse_measurement,
    require_float,
    require_string,
)


class NumericalValidationCampaign:
    def load_profile(self, root: Path, profile: str) -> dict[str, object]:
        payload = json.loads((root / "docs/quality/manifest/numerical_campaign.json").read_text(encoding="utf-8"))
        profiles = payload.get("profiles")
        if not isinstance(profiles, dict):
            raise NumericalValidationError("numerical campaign profiles payload is unavailable")
        profile_payload = profiles.get(profile)
        if not isinstance(profile_payload, dict):
            raise NumericalValidationError(f"unknown numerical validation profile: {profile}")
        return profile_payload

    def run(
        self,
        root: Path,
        dist_dir: Path,
        profile: str,
        tool_path: Path,
    ) -> tuple[Path, dict[str, object]]:
        profile_payload = self.load_profile(root.resolve(), profile)
        runs = self._collect_runs(tool_path.resolve(), profile_payload.get("runs"))
        report = self._build_report(profile, runs, profile_payload.get("comparisons"))
        archive = self._archive(dist_dir.resolve(), report)
        return archive, report

    def _collect_runs(self, tool_path: Path, raw_runs: object) -> dict[str, dict[str, object]]:
        if not isinstance(raw_runs, list):
            raise NumericalValidationError("numerical validation runs must be a list")
        rows: dict[str, dict[str, object]] = {}
        for entry in raw_runs:
            if not isinstance(entry, dict):
                raise NumericalValidationError("numerical validation run entries must be objects")
            run_id = require_string(entry, "id")
            completed = subprocess.run(
                [str(tool_path), "--preset", require_string(entry, "preset"), "--solver", require_string(entry, "solver")],
                capture_output=True,
                text=True,
                check=False,
            )
            if completed.returncode != 0:
                raise NumericalValidationError(f"{run_id}: tool execution failed: {completed.stderr.strip() or completed.stdout.strip()}")
            measurement = parse_measurement(completed.stdout)
            measurement["id"] = run_id
            measurement["preset"] = entry["preset"]
            measurement["solver"] = entry["solver"]
            measurement["dataset"] = entry.get("dataset", measurement.get("dataset", ""))
            measurement["seed"] = entry.get("seed", measurement.get("seed", 0))
            measurement["checks"] = as_mapping(entry.get("checks"))
            rows[run_id] = measurement
        return rows

    def _build_report(self, profile: str, runs: dict[str, dict[str, object]], raw_comparisons: object) -> dict[str, object]:
        failures: list[str] = []
        run_rows = [self._evaluate_run(run, failures) for run in runs.values()]
        comparison_rows = self._evaluate_comparisons(runs, raw_comparisons, failures)
        return {
            "format_version": "1.0",
            "generated_at_utc": datetime.now(UTC).isoformat(timespec="seconds"),
            "profile": profile,
            "status": "failed" if failures else "passed",
            "runs": run_rows,
            "comparisons": comparison_rows,
            "failures": failures,
        }

    def _evaluate_run(self, run: dict[str, object], failures: list[str]) -> dict[str, object]:
        checks = as_mapping(run.get("checks"))
        results = []
        for metric_name, threshold in sorted(checks.items()):
            value = require_float(run, metric_name)
            passed = value <= float(threshold)
            results.append({"metric": metric_name, "value": value, "threshold": float(threshold), "passed": passed})
            if not passed:
                failures.append(f"{run['id']}: {metric_name}={value:.6f} exceeds {float(threshold):.6f}")
        return {"id": run["id"], "preset": run["preset"], "solver": run["solver"], "dataset": run["dataset"], "seed": run["seed"], "checks": results}

    def _evaluate_comparisons(
        self,
        runs: dict[str, dict[str, object]],
        raw_comparisons: object,
        failures: list[str],
    ) -> list[dict[str, object]]:
        if not isinstance(raw_comparisons, list):
            raise NumericalValidationError("numerical validation comparisons must be a list")
        rows: list[dict[str, object]] = []
        for entry in raw_comparisons:
            if not isinstance(entry, dict):
                raise NumericalValidationError("numerical validation comparison entries must be objects")
            baseline = runs[require_string(entry, "baseline")]
            candidate = runs[require_string(entry, "candidate")]
            check_rows = []
            for metric_name, threshold in sorted(as_mapping(entry.get("checks")).items()):
                value = float(comparison_metric(metric_name, baseline, candidate))
                passed = value <= float(threshold)
                check_rows.append({"metric": metric_name, "value": value, "threshold": float(threshold), "passed": passed})
                if not passed:
                    failures.append(f"{entry['id']}: {metric_name}={value:.6f} exceeds {float(threshold):.6f}")
            rows.append({"id": entry["id"], "baseline": entry["baseline"], "candidate": entry["candidate"], "checks": check_rows})
        return rows

    def _archive(self, dist_dir: Path, report: dict[str, object]) -> Path:
        if dist_dir.exists():
            shutil.rmtree(dist_dir)
        dist_dir.mkdir(parents=True, exist_ok=True)
        (dist_dir / "numerical_validation_report.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
        (dist_dir / "README.md").write_text(render_numerical_validation_readme(report), encoding="utf-8")
        archive_base = dist_dir / "CUDA-GRAVITY-SIMULATION-numerical-validation"
        return Path(shutil.make_archive(str(archive_base), "zip", root_dir=dist_dir))

