# File: tests/checks/suites/core/test_release_sbom.py
# Purpose: Verification coverage for the BLITZAR quality gate.

from __future__ import annotations

import json
from pathlib import Path

from python_tools.ci.release_sbom import ReleaseSbomPackager


# Description: Executes the test_release_sbom_packages_release_artifacts operation.
def test_release_sbom_packages_release_artifacts(tmp_path: Path) -> None:
    artifacts_dir = tmp_path / "dist" / "release-bundle"
    artifacts_dir.mkdir(parents=True, exist_ok=True)
    sample = artifacts_dir / "blitzar.exe"
    sample.write_bytes(b"gravity")
    readme = artifacts_dir / "README.md"
    readme.write_text("bundle\n", encoding="utf-8")

    packager = ReleaseSbomPackager()
    output = packager.package(artifacts_dir, tmp_path / "dist" / "release-sbom", "v1.2.3")

    payload = json.loads(output.read_text(encoding="utf-8"))
    assert payload["bomFormat"] == "CycloneDX"
    assert payload["metadata"]["component"]["version"] == "v1.2.3"
    components = payload["components"]
    assert len(components) == 2
    assert any(component["bom-ref"] == "blitzar.exe" for component in components)
    assert any(component["bom-ref"] == "README.md" for component in components)


# Description: Executes the test_release_sbom_requires_artifacts_dir operation.
def test_release_sbom_requires_artifacts_dir(tmp_path: Path) -> None:
    packager = ReleaseSbomPackager()

    try:
        packager.package(tmp_path / "missing", tmp_path / "dist" / "release-sbom", "manual")
    except FileNotFoundError as exc:
        assert "artifacts directory not found" in str(exc)
    else:
        raise AssertionError("expected FileNotFoundError")
