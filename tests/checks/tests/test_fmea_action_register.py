#!/usr/bin/env python3
from __future__ import annotations

import json
from pathlib import Path

import pytest

from python_tools.policies.fmea_action_register import FmeaActionRegister, FmeaActionRegisterError


def _write_register(root: Path, rows: list[dict[str, object]]) -> None:
    path = root / "docs/quality/manifest/fmea_actions.json"
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps({"fmea_actions": rows}, indent=2), encoding="utf-8")


def test_fmea_action_register_accepts_linked_medium_risk(tmp_path: Path) -> None:
    _write_register(
        tmp_path,
        [
            {
                "id": "FMEA-001",
                "owner": "maintainer",
                "status": "closed",
                "residual_risk": "Low",
                "linked_tasks": [],
                "verification_evidence": ["EVD_TEST"],
            },
            {
                "id": "FMEA-002",
                "owner": "maintainer",
                "status": "in-progress",
                "residual_risk": "Medium",
                "linked_tasks": ["#102"],
                "verification_evidence": ["EVD_TEST"],
            },
        ],
    )
    rows = FmeaActionRegister().load(tmp_path)
    assert [row["id"] for row in rows] == ["FMEA-001", "FMEA-002"]


def test_fmea_action_register_rejects_medium_risk_without_tasks(tmp_path: Path) -> None:
    _write_register(
        tmp_path,
        [
            {
                "id": "FMEA-002",
                "owner": "maintainer",
                "status": "in-progress",
                "residual_risk": "Medium",
                "linked_tasks": [],
                "verification_evidence": ["EVD_TEST"],
            }
        ],
    )
    with pytest.raises(FmeaActionRegisterError):
        FmeaActionRegister().load(tmp_path)


def test_fmea_action_register_rejects_closed_row_without_evidence(tmp_path: Path) -> None:
    _write_register(
        tmp_path,
        [
            {
                "id": "FMEA-001",
                "owner": "maintainer",
                "status": "closed",
                "residual_risk": "Low",
                "linked_tasks": [],
                "verification_evidence": [],
            }
        ],
    )
    with pytest.raises(FmeaActionRegisterError):
        FmeaActionRegister().load(tmp_path)
