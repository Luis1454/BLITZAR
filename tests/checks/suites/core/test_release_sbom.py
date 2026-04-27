# @file tests/checks/suites/core/test_release_sbom.py
# @author Luis1454
# @project BLITZAR
# @brief Automated verification assets for BLITZAR quality gates.

from __future__ import annotations

import json
from pathlib import Path

from python_tools.ci.release_sbom import ReleaseSbomPackager


# @brief Documents the test release sbom packages release artifacts operation contract.
# @param tmp_path Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
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


# @brief Documents the test release sbom requires artifacts dir operation contract.
# @param tmp_path Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def test_release_sbom_requires_artifacts_dir(tmp_path: Path) -> None:
    packager = ReleaseSbomPackager()

    try:
        packager.package(tmp_path / "missing", tmp_path / "dist" / "release-sbom", "manual")
    except FileNotFoundError as exc:
        assert "artifacts directory not found" in str(exc)
    else:
        raise AssertionError("expected FileNotFoundError")
