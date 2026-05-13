# @file python_tools/ci/fmea_status.py
# @author Luis1454
# @project BLITZAR
# @brief Python quality and automation support for BLITZAR governance.

from __future__ import annotations

import json
import shutil
from collections.abc import Mapping, Sequence
from datetime import UTC, datetime
from pathlib import Path

from python_tools.policies.fmea_action_register import FmeaActionRegister


# @brief Defines the fmea status snapshot type contract.
# @param None This contract does not take explicit parameters.
# @note Keep construction and side effects explicit for deterministic quality gates.
class FmeaStatusSnapshot:
    # @brief Documents the init operation contract.
    # @param None This contract does not take explicit parameters.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def __init__(self) -> None:
        self._register = FmeaActionRegister()

    # @brief Documents the package operation contract.
    # @param root Input value used by this contract.
    # @param dist_dir Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def package(self, root: Path, dist_dir: Path) -> Path:
        rows = self._register.load(root.resolve())
        snapshot = self._build_snapshot(rows)
        return self._archive(dist_dir.resolve(), snapshot)

    # @brief Documents the build snapshot operation contract.
    # @param rows Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _build_snapshot(self, rows: list[dict[str, object]]) -> dict[str, object]:
        open_count = sum(1 for row in rows if row["status"] == "open")
        in_progress_count = sum(1 for row in rows if row["status"] == "in-progress")
        closed_count = sum(1 for row in rows if row["status"] == "closed")
        high_medium = [row for row in rows if row["residual_risk"] in {"High", "Medium"}]
        return {
            "format_version": "1.0",
            "generated_at_utc": datetime.now(UTC).isoformat(timespec="seconds"),
            "summary": {
                "open": open_count,
                "in_progress": in_progress_count,
                "closed": closed_count,
            },
            "high_medium": high_medium,
            "rows": rows,
        }

    # @brief Documents the archive operation contract.
    # @param dist_dir Input value used by this contract.
    # @param snapshot Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _archive(self, dist_dir: Path, snapshot: dict[str, object]) -> Path:
        if dist_dir.exists():
            shutil.rmtree(dist_dir)
        dist_dir.mkdir(parents=True, exist_ok=True)
        (dist_dir / "fmea_status_snapshot.json").write_text(json.dumps(snapshot, indent=2), encoding="utf-8")
        (dist_dir / "README.md").write_text(render_fmea_status_readme(snapshot), encoding="utf-8")
        archive_base = dist_dir / "ASTER-fmea-status"
        return Path(shutil.make_archive(str(archive_base), "zip", root_dir=dist_dir))


# @brief Documents the render fmea status readme operation contract.
# @param snapshot Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def render_fmea_status_readme(snapshot: Mapping[str, object]) -> str:
    summary = snapshot.get("summary")
    if not isinstance(summary, dict):
        summary = {}
    high_medium = snapshot.get("high_medium")
    if not isinstance(high_medium, Sequence):
        high_medium = ()
    lines = [
        "# FMEA Risk Status Snapshot",
        "",
        f"- Open: `{summary.get('open', 0)}`",
        f"- In Progress: `{summary.get('in_progress', 0)}`",
        f"- Closed: `{summary.get('closed', 0)}`",
        f"- High/Medium residual risks: `{len(high_medium)}`",
        "",
        "See `fmea_status_snapshot.json` for machine-readable details.",
    ]
    return "\n".join(lines) + "\n"
