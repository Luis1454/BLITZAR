#!/usr/bin/env python3
from __future__ import annotations

import json
import sys
import zipfile
from pathlib import Path

from python_tools.ci.numerical_validation import NumericalValidationCampaign
from python_tools.ci.release_bundle import ReleaseBundlePackager
from python_tools.ci.tool_manifest import ToolManifestCollector


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
            row.update({"id": entry["id"], "preset": entry["preset"], "solver": entry["solver"], "dataset": entry["dataset"], "seed": entry["seed"], "checks": entry.get("checks", {})})
            rows[entry["id"]] = row
        return rows


def _fake_runner(outputs: dict[tuple[str, ...], str]):
    def run(command: list[str]) -> str:
        key = tuple(command)
        if key not in outputs:
            raise RuntimeError(f"missing mock output for {' '.join(command)}")
        return outputs[key]

    return run


def _write_campaign(root: Path) -> None:
    payload = {
        "profiles": {
            "gpu-prod": {
                "runs": [
                    {"id": "energy", "preset": "two_body_orbit_drift", "solver": "pairwise_cuda", "dataset": "tests/data/two_body_rest.xyz", "seed": 0, "checks": {"max_abs_energy_drift_pct": 5.0, "center_of_mass_drift": 0.01}},
                    {"id": "fine", "preset": "two_body_orbit_convergence_fine", "solver": "pairwise_cuda", "dataset": "tests/data/two_body_rest.xyz", "seed": 0},
                    {"id": "coarse", "preset": "two_body_orbit_convergence_coarse", "solver": "pairwise_cuda", "dataset": "tests/data/two_body_rest.xyz", "seed": 0},
                ],
                "comparisons": [{"id": "convergence", "baseline": "fine", "candidate": "coarse", "checks": {"max_particle_delta": 0.05}}],
            }
        }
    }
    path = root / "docs/quality/manifest/numerical_campaign.json"
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2), encoding="utf-8")


def test_release_bundle_embeds_tool_manifest(tmp_path: Path, monkeypatch) -> None:
    monkeypatch.chdir(tmp_path)
    build_dir = tmp_path / "build"
    build_dir.mkdir()
    (build_dir / "myApp.exe").write_text("binary\n", encoding="utf-8")
    (tmp_path / "simulation.ini").write_text("config\n", encoding="utf-8")
    (tmp_path / "README.md").write_text("readme\n", encoding="utf-8")
    tool_manifest = tmp_path / "dist/tool-qualification/tool_manifest.json"
    tool_manifest.parent.mkdir(parents=True, exist_ok=True)
    tool_manifest.write_text('{"lane":"release"}\n', encoding="utf-8")
    archive = ReleaseBundlePackager().package(build_dir, tmp_path / "dist/release-bundle", "rc-1", tool_manifest)
    with zipfile.ZipFile(archive) as bundle:
        names = bundle.namelist()
        assert "tool_manifest.json" in names
        assert "myApp.exe" in names


def test_tool_manifest_collector_records_versions_and_missing_tools(tmp_path: Path) -> None:
    collector = ToolManifestCollector(
        _fake_runner(
            {
                (sys.executable, "--version"): "Python 3.12.0",
                ("cmake", "--version"): "cmake version 3.30.1",
                ("clang-tidy", "--version"): "LLVM clang-tidy 18.1.8",
                ("c++", "--version"): "g++ 14.2.0",
            }
        )
    )
    manifest = collector.collect(lane="pr-fast", profile="prod", compiler_command=["c++", "--version"])
    tools = manifest["tools"]
    assert isinstance(tools, dict)
    assert tools["python"]["status"] == "available"
    assert tools["compiler"]["version"] == "g++ 14.2.0"
    saved = json.loads(collector.write(manifest, tmp_path / "tool_manifest.json").read_text(encoding="utf-8"))
    assert saved["profile"] == "prod"

    manifest = ToolManifestCollector(_fake_runner({(sys.executable, "--version"): "Python 3.12.0"})).collect(
        lane="release",
        profile="prod",
        compiler_command=["c++", "--version"],
    )
    tools = manifest["tools"]
    assert isinstance(tools, dict)
    assert tools["cmake"]["status"] == "unavailable"
    assert tools["clang_tidy"]["status"] == "unavailable"


def test_numerical_validation_passes_when_thresholds_hold(tmp_path: Path) -> None:
    _write_campaign(tmp_path)
    campaign = FakeNumericalValidationCampaign(
        {
            "energy": {"max_abs_energy_drift_pct": 1.0, "center_of_mass_drift": 0.001, "average_radius": 1.0, "total_energy": -10.0, "final_center_of_mass": [0.0, 0.0, 0.0], "final_particles": [[0.0, 0.0, 0.0], [1.0, 0.0, 0.0]]},
            "fine": {"average_radius": 1.0, "total_energy": -10.0, "final_center_of_mass": [0.0, 0.0, 0.0], "final_particles": [[0.0, 0.0, 0.0], [1.0, 0.0, 0.0]]},
            "coarse": {"average_radius": 1.0, "total_energy": -10.0, "final_center_of_mass": [0.0, 0.0, 0.0], "final_particles": [[0.0, 0.0, 0.0], [1.02, 0.0, 0.0]]},
        }
    )
    _, report = campaign.run(tmp_path, tmp_path / "dist/numerical", "gpu-prod", tmp_path / "fake-tool")
    assert report["status"] == "passed"
    assert report["failures"] == []


def test_numerical_validation_fails_when_thresholds_are_exceeded(tmp_path: Path) -> None:
    _write_campaign(tmp_path)
    campaign = FakeNumericalValidationCampaign(
        {
            "energy": {"max_abs_energy_drift_pct": 8.0, "center_of_mass_drift": 0.02, "average_radius": 1.0, "total_energy": -10.0, "final_center_of_mass": [0.0, 0.0, 0.0], "final_particles": [[0.0, 0.0, 0.0], [1.0, 0.0, 0.0]]},
            "fine": {"average_radius": 1.0, "total_energy": -10.0, "final_center_of_mass": [0.0, 0.0, 0.0], "final_particles": [[0.0, 0.0, 0.0], [1.0, 0.0, 0.0]]},
            "coarse": {"average_radius": 1.0, "total_energy": -10.0, "final_center_of_mass": [0.0, 0.0, 0.0], "final_particles": [[0.0, 0.0, 0.0], [1.1, 0.0, 0.0]]},
        }
    )
    _, report = campaign.run(tmp_path, tmp_path / "dist/numerical", "gpu-prod", tmp_path / "fake-tool")
    assert report["status"] == "failed"
    assert isinstance(report["failures"], list)
    assert len(report["failures"]) == 3
