from __future__ import annotations

import json
import shutil
import subprocess
from collections.abc import Mapping, Sequence
from datetime import UTC, datetime
from pathlib import Path
from typing import cast


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

    def run(self, root: Path, dist_dir: Path, profile: str, tool_path: Path) -> tuple[Path, dict[str, object]]:
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
            command = [str(tool_path), "--preset", require_string(entry, "preset"), "--solver", require_string(entry, "solver")]
            completed = subprocess.run(command, capture_output=True, text=True, check=False)
            if completed.returncode != 0:
                raise NumericalValidationError(f"{run_id}: tool execution failed: {completed.stderr.strip() or completed.stdout.strip()}")
            measurement = parse_measurement(completed.stdout)
            measurement.update(
                {
                    "id": run_id,
                    "preset": entry["preset"],
                    "solver": entry["solver"],
                    "dataset": entry.get("dataset", measurement.get("dataset", "")),
                    "seed": entry.get("seed", measurement.get("seed", 0)),
                    "checks": as_mapping(entry.get("checks")),
                }
            )
            rows[run_id] = measurement
        return rows

    def _build_report(self, profile: str, runs: dict[str, dict[str, object]], raw_comparisons: object) -> dict[str, object]:
        failures: list[str] = []
        run_rows = [self._evaluate_run(run, failures) for run in runs.values()]
        comparison_rows = self._evaluate_comparisons(runs, raw_comparisons, failures)
        return {"format_version": "1.0", "generated_at_utc": datetime.now(UTC).isoformat(timespec="seconds"), "profile": profile, "status": "failed" if failures else "passed", "runs": run_rows, "comparisons": comparison_rows, "failures": failures}

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

    def _evaluate_comparisons(self, runs: dict[str, dict[str, object]], raw_comparisons: object, failures: list[str]) -> list[dict[str, object]]:
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
        archive_base = dist_dir / "ASTER-numerical-validation"
        return Path(shutil.make_archive(str(archive_base), "zip", root_dir=dist_dir))


class NumericalValidationError(RuntimeError):
    pass


def render_numerical_validation_readme(report: Mapping[str, object]) -> str:
    run_rows = _as_sequence(report.get("runs"))
    comparison_rows = _as_sequence(report.get("comparisons"))
    failures = _as_sequence(report.get("failures"))
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


def parse_measurement(stdout: str) -> dict[str, object]:
    particles: list[list[float]] = []
    result: dict[str, object] = {"final_particles": particles}
    for raw_line in stdout.splitlines():
        line = raw_line.strip()
        if not line or "=" not in line:
            continue
        name, raw_value = line.split("=", 1)
        if name.startswith("final_particle_"):
            particles.append([float(part) for part in raw_value.split(",")])
            continue
        if name in {"initial_center_of_mass", "final_center_of_mass"}:
            result[name] = [float(part) for part in raw_value.split(",")]
            continue
        if name in {"seed", "steps_completed", "particle_count"}:
            result[name] = int(raw_value)
            continue
        if name == "energy_estimated":
            result[name] = raw_value == "1"
            continue
        try:
            result[name] = float(raw_value)
        except ValueError:
            result[name] = raw_value
    return result


def comparison_metric(metric_name: str, baseline: dict[str, object], candidate: dict[str, object]) -> float:
    if metric_name == "center_of_mass_delta":
        return vector_distance(baseline, candidate, "final_center_of_mass")
    if metric_name == "average_radius_delta":
        return abs(require_float(candidate, "average_radius") - require_float(baseline, "average_radius"))
    if metric_name == "total_energy_diff_pct":
        return percent_delta(require_float(baseline, "total_energy"), require_float(candidate, "total_energy"))
    if metric_name == "max_particle_delta":
        return max_particle_delta(baseline, candidate)
    raise NumericalValidationError(f"unsupported comparison metric: {metric_name}")


def as_mapping(raw: object) -> dict[str, float]:
    if raw is None:
        return {}
    if not isinstance(raw, dict):
        raise NumericalValidationError("numerical validation threshold payload must be an object")
    return {str(name): float(value) for name, value in raw.items()}


def require_string(row: dict[str, object], field: str) -> str:
    value = row.get(field)
    if not isinstance(value, str) or not value.strip():
        raise NumericalValidationError(f"missing required field: {field}")
    return value.strip()


def require_float(row: dict[str, object], field: str) -> float:
    value = row.get(field)
    if isinstance(value, (int, float)):
        return float(value)
    raise NumericalValidationError(f"missing numeric field: {field}")


def vector_distance(row_a: dict[str, object], row_b: dict[str, object], field: str) -> float:
    return _distance(_vector3(row_a, field), _vector3(row_b, field))


def max_particle_delta(baseline: dict[str, object], candidate: dict[str, object]) -> float:
    particles_a = baseline.get("final_particles")
    particles_b = candidate.get("final_particles")
    if not isinstance(particles_a, list) or not isinstance(particles_b, list) or len(particles_a) != len(particles_b) or not particles_a:
        raise NumericalValidationError("final particle payload is unavailable for convergence comparison")
    rows_a = cast(list[list[object]], particles_a)
    rows_b = cast(list[list[object]], particles_b)
    return max(_distance(_row3(row_a), _row3(row_b)) for row_a, row_b in zip(rows_a, rows_b, strict=True))


def percent_delta(baseline: float, candidate: float) -> float:
    return abs(candidate - baseline) / max(abs(baseline), 1e-6) * 100.0


def coerce_float(value: object) -> float:
    if isinstance(value, (int, float)):
        return float(value)
    raise NumericalValidationError("numerical vector entries must be numeric")


def _vector3(row: dict[str, object], field: str) -> list[float]:
    vector = row.get(field)
    if not isinstance(vector, list) or len(vector) != 3:
        raise NumericalValidationError(f"missing vector field for comparison: {field}")
    return [coerce_float(value) for value in cast(list[object], vector)]


def _row3(row: list[object]) -> list[float]:
    return [coerce_float(value) for value in row]


def _distance(values_a: Sequence[float], values_b: Sequence[float]) -> float:
    dx = values_a[0] - values_b[0]
    dy = values_a[1] - values_b[1]
    dz = values_a[2] - values_b[2]
    return float((dx * dx + dy * dy + dz * dz) ** 0.5)


def _as_sequence(value: object) -> Sequence[object]:
    return value if isinstance(value, Sequence) and not isinstance(value, (str, bytes, bytearray)) else ()
