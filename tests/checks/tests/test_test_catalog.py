#!/usr/bin/env python3
from __future__ import annotations

import json
from pathlib import Path

from python_tools.core.models import CheckContext
from python_tools.policies.test_catalog import TestCatalogCheck


def _write(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def _seed_requirements(root: Path) -> None:
    payload = {"system": "test", "revision": "2026-03-01", "requirements": [{"id": "REQ-TEST-001", "title": "Cataloged tests", "verification": {"tests": [".*"], "artifacts": []}}]}
    _write(root / "docs/quality/requirements.json", json.dumps(payload) + "\n")


def test_test_catalog_passes_with_matching_known_test(tmp_path: Path) -> None:
    _seed_requirements(tmp_path)
    _write(tmp_path / "tests/unit/sample.cpp", "TEST(SampleSuite, SampleCase) {}\n")
    _write(tmp_path / "docs/quality/test_catalog.csv", "test_code,test_id,req_ids,source\n" "TST-UNT-SAMP-001,SampleSuite.SampleCase,REQ-TEST-001,tests/unit/sample.cpp\n")
    result = TestCatalogCheck().run(CheckContext(root=tmp_path, options={"extra_test_ids": set()}))
    assert result.ok
    assert result.errors == []


def test_test_catalog_fails_on_unknown_test_id(tmp_path: Path) -> None:
    _seed_requirements(tmp_path)
    _write(tmp_path / "tests/unit/sample.cpp", "TEST(SampleSuite, SampleCase) {}\n")
    _write(tmp_path / "docs/quality/test_catalog.csv", "test_code,test_id,req_ids,source\n" "TST-UNT-SAMP-001,MissingSuite.MissingCase,REQ-TEST-001,tests/unit/sample.cpp\n")
    result = TestCatalogCheck().run(CheckContext(root=tmp_path, options={"extra_test_ids": set()}))
    assert not result.ok
    assert any("unknown test_id" in error for error in result.errors)
