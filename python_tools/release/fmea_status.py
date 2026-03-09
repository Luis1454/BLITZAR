from __future__ import annotations

import json
import shutil
from datetime import UTC, datetime
from pathlib import Path

from python_tools.checks.fmea_action_register import FmeaActionRegister
from python_tools.release.fmea_status_assets import render_fmea_status_readme


class FmeaStatusSnapshot:
    def __init__(self) -> None:
        self._register = FmeaActionRegister()

    def package(self, root: Path, dist_dir: Path) -> Path:
        rows = self._register.load(root.resolve())
        snapshot = self._build_snapshot(rows)
        return self._archive(dist_dir.resolve(), snapshot)

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

    def _archive(self, dist_dir: Path, snapshot: dict[str, object]) -> Path:
        if dist_dir.exists():
            shutil.rmtree(dist_dir)
        dist_dir.mkdir(parents=True, exist_ok=True)
        (dist_dir / "fmea_status_snapshot.json").write_text(json.dumps(snapshot, indent=2), encoding="utf-8")
        (dist_dir / "README.md").write_text(render_fmea_status_readme(snapshot), encoding="utf-8")
        archive_base = dist_dir / "CUDA-GRAVITY-SIMULATION-fmea-status"
        return Path(shutil.make_archive(str(archive_base), "zip", root_dir=dist_dir))

