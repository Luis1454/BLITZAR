#!/usr/bin/env python3
from __future__ import annotations

import json
from pathlib import Path

from python_tools.ci.numerical_validation import NumericalValidationCampaign


class FakeNumericalValidationCampaign(NumericalValidationCampaign):
    def __init__(self, measurements: dict[str, dict[str, object]]) -> None:
        self._measurements = measurements

    def _collect_runs(self, tool_path: Path, raw_runs: object) -> dict[str, dict[str, object]]:
        del tool_path
        assert isinstance(raw_runs, list)
        rows: dict[str, dict[str, object]] = {}
        for entry in raw_runs:
            assert isinstance(entry, dict)
            row = dict(self._measurements[entry["id"]])
            row["id"] = entry["id"]
            row["preset"] = entry["preset"]
            row["solver"] = entry["solver"]
            row["dataset"] = entry["dataset"]
            row["seed"] = entry["seed"]
            row["checks"] = entry.get("checks", {})
            rows[entry["id"]] = row
        return rows


def _write_campaign(root: Path) -> None:
    payload = {
        "profiles": {
            "gpu-prod": {
                "runs": [
                    {
                        "id": "energy",
                        "preset": "two_body_orbit_drift",
                        "solver": "pairwise_cuda",
                        "dataset": "tests/data/two_body_rest.xyz",
                        "seed": 0,
                        "checks": {
                            "max_abs_energy_drift_pct": 5.0,
                            "center_of_mass_drift": 0.01,
                        },
                    },
                    {
                        "id": "fine",
                        "preset": "two_body_orbit_convergence_fine",
                        "solver": "pairwise_cuda",
                        "dataset": "tests/data/two_body_rest.xyz",
                        "seed": 0,
                    },
                    {
                        "id": "coarse",
                        "preset": "two_body_orbit_convergence_coarse",
                        "solver": "pairwise_cuda",
                        "dataset": "tests/data/two_body_rest.xyz",
                        "seed": 0,
                    },
                ],
                "comparisons": [
                    {
                        "id": "convergence",
                        "baseline": "fine",
                        "candidate": "coarse",
                        "checks": {"max_particle_delta": 0.05},
                    }
                ],
            }
        }
    }
    target = root / "docs/quality/manifest/numerical_campaign.json"
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(payload, indent=2), encoding="utf-8")


def test_numerical_validation_passes_when_thresholds_hold(tmp_path: Path) -> None:
    _write_campaign(tmp_path)
    campaign = FakeNumericalValidationCampaign(
        {
            "energy": {
                "max_abs_energy_drift_pct": 1.0,
                "center_of_mass_drift": 0.001,
                "average_radius": 1.0,
                "total_energy": -10.0,
                "final_center_of_mass": [0.0, 0.0, 0.0],
                "final_particles": [[0.0, 0.0, 0.0], [1.0, 0.0, 0.0]],
            },
            "fine": {
                "average_radius": 1.0,
                "total_energy": -10.0,
                "final_center_of_mass": [0.0, 0.0, 0.0],
                "final_particles": [[0.0, 0.0, 0.0], [1.0, 0.0, 0.0]],
            },
            "coarse": {
                "average_radius": 1.0,
                "total_energy": -10.0,
                "final_center_of_mass": [0.0, 0.0, 0.0],
                "final_particles": [[0.0, 0.0, 0.0], [1.02, 0.0, 0.0]],
            },
        }
    )
    _, report = campaign.run(tmp_path, tmp_path / "dist/numerical", "gpu-prod", tmp_path / "fake-tool")
    assert report["status"] == "passed"
    assert report["failures"] == []


def test_numerical_validation_fails_when_thresholds_are_exceeded(tmp_path: Path) -> None:
    _write_campaign(tmp_path)
    campaign = FakeNumericalValidationCampaign(
        {
            "energy": {
                "max_abs_energy_drift_pct": 8.0,
                "center_of_mass_drift": 0.02,
                "average_radius": 1.0,
                "total_energy": -10.0,
                "final_center_of_mass": [0.0, 0.0, 0.0],
                "final_particles": [[0.0, 0.0, 0.0], [1.0, 0.0, 0.0]],
            },
            "fine": {
                "average_radius": 1.0,
                "total_energy": -10.0,
                "final_center_of_mass": [0.0, 0.0, 0.0],
                "final_particles": [[0.0, 0.0, 0.0], [1.0, 0.0, 0.0]],
            },
            "coarse": {
                "average_radius": 1.0,
                "total_energy": -10.0,
                "final_center_of_mass": [0.0, 0.0, 0.0],
                "final_particles": [[0.0, 0.0, 0.0], [1.1, 0.0, 0.0]],
            },
        }
    )
    _, report = campaign.run(tmp_path, tmp_path / "dist/numerical", "gpu-prod", tmp_path / "fake-tool")
    assert report["status"] == "failed"
    assert isinstance(report["failures"], list)
    assert len(report["failures"]) == 3
