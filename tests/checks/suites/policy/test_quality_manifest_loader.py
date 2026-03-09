#!/usr/bin/env python3
from __future__ import annotations

import json
from pathlib import Path

from python_tools.checks.quality_manifest import QualityManifestLoader
from python_tools.core.models import CheckResult


def _write(path: Path, payload: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload), encoding="utf-8")


def test_quality_manifest_loader_merges_includes(tmp_path: Path) -> None:
    _write(tmp_path / "docs/quality/manifest/evidence.json", {"evidence": {"EVD_A": "a.txt"}})
    _write(tmp_path / "docs/quality/manifest/requirements.json", {"requirements": {"REQ-TEST-001": {"tests": [".*"], "artifacts": ["EVD_A"]}}})
    _write(
        tmp_path / "docs/quality/quality_manifest.json",
        {
            "metadata": {"system": "test", "revision": "2026-03-02"},
            "includes": ["manifest/evidence.json", "manifest/requirements.json"],
        },
    )

    result = CheckResult(name="manifest")
    payload = QualityManifestLoader().load(tmp_path, result)
    assert result.errors == []
    assert "metadata" in payload
    assert "evidence" in payload
    assert "requirements" in payload


def test_quality_manifest_loader_rejects_missing_include(tmp_path: Path) -> None:
    _write(
        tmp_path / "docs/quality/quality_manifest.json",
        {
            "metadata": {"system": "test", "revision": "2026-03-02"},
            "includes": ["manifest/missing.json"],
        },
    )

    result = CheckResult(name="manifest")
    _ = QualityManifestLoader().load(tmp_path, result)
    assert any("missing required quality manifest" in error for error in result.errors)


def test_quality_manifest_loader_rejects_duplicate_top_level_keys(tmp_path: Path) -> None:
    _write(tmp_path / "docs/quality/manifest/a.json", {"evidence": {"EVD_A": "a.txt"}})
    _write(tmp_path / "docs/quality/manifest/b.json", {"evidence": {"EVD_B": "b.txt"}})
    _write(
        tmp_path / "docs/quality/quality_manifest.json",
        {
            "metadata": {"system": "test", "revision": "2026-03-02"},
            "includes": ["manifest/a.json", "manifest/b.json"],
        },
    )

    result = CheckResult(name="manifest")
    _ = QualityManifestLoader().load(tmp_path, result)
    assert any("duplicate top-level key 'evidence'" in error for error in result.errors)


def test_quality_manifest_loader_rejects_include_cycle(tmp_path: Path) -> None:
    _write(
        tmp_path / "docs/quality/quality_manifest.json",
        {
            "metadata": {"system": "test", "revision": "2026-03-02"},
            "includes": ["manifest/a.json"],
        },
    )
    _write(tmp_path / "docs/quality/manifest/a.json", {"includes": ["b.json"]})
    _write(tmp_path / "docs/quality/manifest/b.json", {"includes": ["a.json"]})

    result = CheckResult(name="manifest")
    _ = QualityManifestLoader().load(tmp_path, result)
    assert any("include cycle detected" in error for error in result.errors)
