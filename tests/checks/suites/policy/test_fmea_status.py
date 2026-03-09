#!/usr/bin/env python3
from __future__ import annotations

import json
from pathlib import Path

from python_tools.release.fmea_status import FmeaStatusSnapshot


def _write_register(root: Path) -> None:
    payload = {
        "fmea_actions": [
            {
                "id": "FMEA-001",
                "owner": "config-maintainer",
                "status": "closed",
                "residual_risk": "Low",
                "linked_tasks": [],
                "verification_evidence": ["EVD_TEST"],
            },
            {
                "id": "FMEA-002",
                "owner": "runtime-maintainer",
                "status": "in-progress",
                "residual_risk": "Medium",
                "linked_tasks": ["#120"],
                "verification_evidence": ["EVD_TEST"],
            },
        ]
    }
    path = root / "docs/quality/manifest/fmea_actions.json"
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2), encoding="utf-8")


def test_fmea_status_snapshot_packages_summary(tmp_path: Path) -> None:
    _write_register(tmp_path)
    archive = FmeaStatusSnapshot().package(tmp_path, tmp_path / "dist/fmea")
    assert archive.exists()
    snapshot = json.loads((tmp_path / "dist/fmea/fmea_status_snapshot.json").read_text(encoding="utf-8"))
    assert snapshot["summary"] == {"open": 0, "in_progress": 1, "closed": 1}
    assert [row["id"] for row in snapshot["high_medium"]] == ["FMEA-002"]
