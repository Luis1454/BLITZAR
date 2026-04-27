# File: tests/checks/suites/release/test_release_outputs.py
# Purpose: Verification coverage for the BLITZAR quality gate.

from __future__ import annotations

import json
import subprocess
import sys
import zipfile
from pathlib import Path

from python_tools.ci.numerical_validation import NumericalValidationCampaign
from python_tools.ci.release_bundle import ReleaseBundlePackager, ReleaseBundleSmokeValidator
from python_tools.ci.release_source import ReleaseSourcePackager
from python_tools.ci.tool_manifest import ToolManifestCollector


# Description: Defines the FakeNumericalValidationCampaign contract.
class FakeNumericalValidationCampaign(NumericalValidationCampaign):
    # Description: Executes the __init__ operation.
    def __init__(self, measurements: dict[str, dict[str, object]]) -> None:
        self._measurements = measurements

    # Description: Executes the _collect_runs operation.
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


# Description: Executes the _fake_runner operation.
def _fake_runner(outputs: dict[tuple[str, ...], str]):
    # Description: Executes the run operation.
    def run(command: list[str]) -> str:
        key = tuple(command)
        if key not in outputs:
            raise RuntimeError(f"missing mock output for {' '.join(command)}")
        return outputs[key]

    return run


# Description: Executes the _write_campaign operation.
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


# Description: Executes the test_release_bundle_embeds_tool_manifest operation.
def test_release_bundle_embeds_tool_manifest(tmp_path: Path, monkeypatch) -> None:
    monkeypatch.chdir(tmp_path)
    build_dir = tmp_path / "build"
    build_dir.mkdir()
    (build_dir / "blitzar.exe").write_text("binary\n", encoding="utf-8")
    (build_dir / "blitzar-client.exe").write_text("client\n", encoding="utf-8")
    (build_dir / "gravityClientModuleQtInProc.dll").write_text("qt-module\n", encoding="utf-8")
    (build_dir / "gravityClientModuleQtInProc.dll.manifest").write_text("manifest\n", encoding="utf-8")
    (build_dir / "Qt6Core.dll").write_text("qt\n", encoding="utf-8")
    (build_dir / "platforms").mkdir()
    (build_dir / "platforms" / "qwindows.dll").write_text("plugin\n", encoding="utf-8")
    (tmp_path / "simulation.ini").write_text("config\n", encoding="utf-8")
    (tmp_path / "README.md").write_text("readme\n", encoding="utf-8")
    tool_manifest = tmp_path / "dist/tool-qualification/tool_manifest.json"
    tool_manifest.parent.mkdir(parents=True, exist_ok=True)
    tool_manifest.write_text('{"lane":"release"}\n', encoding="utf-8")
    archive = ReleaseBundlePackager().package(build_dir, tmp_path / "dist/release-bundle", "rc-1", tool_manifest)
    assert archive.parent == tmp_path / "dist/release-bundle"
    with zipfile.ZipFile(archive) as bundle:
        names = bundle.namelist()
        assert "tool_manifest.json" in names
        assert "blitzar.exe" in names
        assert "blitzar-client.exe" in names
        assert "gravityClientModuleQtInProc.dll" in names
        assert "gravityClientModuleQtInProc.dll.manifest" in names
        assert "Qt6Core.dll" in names
        assert "platforms/qwindows.dll" in names
        assert archive.name not in names


# Description: Executes the test_release_bundle_smoke_validator_requires_qt_platform_plugin_for_qt_module operation.
def test_release_bundle_smoke_validator_requires_qt_platform_plugin_for_qt_module(tmp_path: Path) -> None:
    bundle_root = tmp_path / "bundle"
    bundle_root.mkdir()
    (bundle_root / "blitzar-client.exe").write_text("client\n", encoding="utf-8")
    (bundle_root / "gravityClientModuleQtInProc.dll").write_text("qt-module\n", encoding="utf-8")
    (bundle_root / "gravityClientModuleQtInProc.dll.manifest").write_text("manifest\n", encoding="utf-8")
    (bundle_root / "Qt6Core.dll").write_text("qt\n", encoding="utf-8")
    (bundle_root / "simulation.ini").write_text("config\n", encoding="utf-8")
    (bundle_root / "README.md").write_text("readme\n", encoding="utf-8")

    try:
        ReleaseBundleSmokeValidator().validate_layout(bundle_root)
        raise AssertionError("expected missing Qt platform plugin to fail validation")
    except RuntimeError as error:
        assert "platforms/" in str(error)


# Description: Executes the test_release_bundle_smoke_validator_runs_help_commands_for_present_binaries operation.
def test_release_bundle_smoke_validator_runs_help_commands_for_present_binaries(tmp_path: Path) -> None:
    bundle_root = tmp_path / "bundle"
    bundle_root.mkdir()
    for name in ("blitzar.exe", "blitzar-headless.exe", "blitzar-server.exe", "blitzar-client.exe"):
        (bundle_root / name).write_text("exe\n", encoding="utf-8")
    (bundle_root / "simulation.ini").write_text("config\n", encoding="utf-8")
    (bundle_root / "README.md").write_text("readme\n", encoding="utf-8")
    seen: list[tuple[list[str], Path]] = []

    # Description: Executes the _runner operation.
    def _runner(command: list[str], cwd: Path) -> None:
        seen.append((command, cwd))

    validator = ReleaseBundleSmokeValidator(_runner)
    validator.validate_layout(bundle_root)
    validator.run_smoke(bundle_root)

    expected = ["blitzar.exe", "blitzar-headless.exe", "blitzar-server.exe", "blitzar-client.exe"]
    assert [Path(command[0]).name for command, _ in seen] == expected
    assert all(cwd == bundle_root for _, cwd in seen)


# Description: Executes the test_release_source_packager_archives_tracked_files_only operation.
def test_release_source_packager_archives_tracked_files_only(tmp_path: Path) -> None:
    subprocess.run(["git", "init"], cwd=tmp_path, check=True, capture_output=True, text=True)
    (tmp_path / "README.md").write_text("readme\n", encoding="utf-8")
    (tmp_path / "untracked.log").write_text("log\n", encoding="utf-8")
    subprocess.run(["git", "add", "README.md"], cwd=tmp_path, check=True)
    commit = ["git", "-c", "user.email=ci@example.invalid", "-c", "user.name=CI", "commit", "-m", "test: seed source archive"]
    subprocess.run(commit, cwd=tmp_path, check=True, capture_output=True, text=True)

    archive = ReleaseSourcePackager().package(tmp_path, tmp_path / "dist/source", "v1.0.0")

    with zipfile.ZipFile(archive) as source:
        names = source.namelist()
    assert "blitzar-v1.0.0-source/README.md" in names
    assert "blitzar-v1.0.0-source/untracked.log" not in names


# Description: Executes the test_tool_manifest_collector_records_versions_and_missing_tools operation.
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


# Description: Executes the test_numerical_validation_passes_when_thresholds_hold operation.
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


# Description: Executes the test_numerical_validation_fails_when_thresholds_are_exceeded operation.
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
