#!/usr/bin/env python3
from __future__ import annotations

import json
from pathlib import Path

from python_tools.core.models import CheckContext
from python_tools.policies.test_catalog import TestCatalogCheck
from tests.checks.suites.support.path_specs import TESTS_UNIT_DIR, cpp_file


def _write(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def _seed_requirements(root: Path) -> None:
    payload = {
        "metadata": {"system": "test", "revision": "2026-03-01"},
        "evidence": {
            "EVD_QLT_MANIFEST": "docs/quality/quality_manifest.json",
            "EVD_CHECK_MAIN": "tests/checks/check.py",
        },
        "requirements": {
            "REQ-TEST-001": {
                "tests": [".*"],
                "artifacts": ["EVD_QLT_MANIFEST"],
            }
        },
        "crosswalk": {
            "CTRL-001": {
                "source_standard": "NPR-7150.2D",
                "artifact": "EVD_QLT_MANIFEST",
                "check": "unit",
            }
        },
        "test_groups": {},
    }
    _write(root / "docs/quality/quality_manifest.json", json.dumps(payload) + "\n")
    _write(root / "tests/checks/check.py", "def main() -> int:\n    return 0\n")


def test_test_catalog_passes_with_matching_known_test(tmp_path: Path) -> None:
    _seed_requirements(tmp_path)
    sample_path = cpp_file(TESTS_UNIT_DIR, "sample")
    _write(tmp_path / sample_path, "TEST(SampleSuite, SampleCase) {}\n")
    payload = json.loads((tmp_path / "docs/quality/quality_manifest.json").read_text(encoding="utf-8"))
    payload["test_groups"] = {
        "EVD_CHECK_MAIN": {
            "TST-UNT-SAMP-001": {
                "id": "SampleSuite.SampleCase",
                "req_ids": ["REQ-TEST-001"],
            }
        }
    }
    _write(tmp_path / "docs/quality/quality_manifest.json", json.dumps(payload) + "\n")
    result = TestCatalogCheck().run(CheckContext(root=tmp_path, options={"extra_test_ids": set()}))
    assert result.ok
    assert result.errors == []


def test_test_catalog_fails_on_unknown_test_id(tmp_path: Path) -> None:
    _seed_requirements(tmp_path)
    sample_path = cpp_file(TESTS_UNIT_DIR, "sample")
    _write(tmp_path / sample_path, "TEST(SampleSuite, SampleCase) {}\n")
    payload = json.loads((tmp_path / "docs/quality/quality_manifest.json").read_text(encoding="utf-8"))
    payload["test_groups"] = {
        "EVD_CHECK_MAIN": {
            "TST-UNT-SAMP-001": {
                "id": "MissingSuite.MissingCase",
                "req_ids": ["REQ-TEST-001"],
            }
        }
    }
    _write(tmp_path / "docs/quality/quality_manifest.json", json.dumps(payload) + "\n")
    result = TestCatalogCheck().run(CheckContext(root=tmp_path, options={"extra_test_ids": set()}))
    assert not result.ok
    assert any("unknown test_id" in error for error in result.errors)
