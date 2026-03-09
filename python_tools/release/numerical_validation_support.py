from __future__ import annotations

from typing import cast


class NumericalValidationError(RuntimeError):
    pass


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
        elif name in {"initial_center_of_mass", "final_center_of_mass"}:
            result[name] = [float(part) for part in raw_value.split(",")]
        elif name in {"seed", "steps_completed", "particle_count"}:
            result[name] = int(raw_value)
        elif name == "energy_estimated":
            result[name] = raw_value == "1"
        else:
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
    vector_a = row_a.get(field)
    vector_b = row_b.get(field)
    if not isinstance(vector_a, list) or not isinstance(vector_b, list) or len(vector_a) != 3 or len(vector_b) != 3:
        raise NumericalValidationError(f"missing vector field for comparison: {field}")
    values_a = [coerce_float(value) for value in cast(list[object], vector_a)]
    values_b = [coerce_float(value) for value in cast(list[object], vector_b)]
    dx = values_a[0] - values_b[0]
    dy = values_a[1] - values_b[1]
    dz = values_a[2] - values_b[2]
    return float((dx * dx + dy * dy + dz * dz) ** 0.5)


def max_particle_delta(baseline: dict[str, object], candidate: dict[str, object]) -> float:
    particles_a = baseline.get("final_particles")
    particles_b = candidate.get("final_particles")
    if not isinstance(particles_a, list) or not isinstance(particles_b, list) or len(particles_a) != len(particles_b) or not particles_a:
        raise NumericalValidationError("final particle payload is unavailable for convergence comparison")
    rows_a = cast(list[list[object]], particles_a)
    rows_b = cast(list[list[object]], particles_b)
    deltas: list[float] = []
    for row_a, row_b in zip(rows_a, rows_b, strict=True):
        coords_a = [coerce_float(value) for value in row_a]
        coords_b = [coerce_float(value) for value in row_b]
        dx = coords_a[0] - coords_b[0]
        dy = coords_a[1] - coords_b[1]
        dz = coords_a[2] - coords_b[2]
        deltas.append(float((dx * dx + dy * dy + dz * dz) ** 0.5))
    return max(deltas)


def percent_delta(baseline: float, candidate: float) -> float:
    return abs(candidate - baseline) / max(abs(baseline), 1e-6) * 100.0


def coerce_float(value: object) -> float:
    if isinstance(value, (int, float)):
        return float(value)
    raise NumericalValidationError("numerical vector entries must be numeric")

