#!/usr/bin/env python3
# File: python_tools/ci/release_source.py
# Purpose: Python quality and automation support for BLITZAR governance.

from __future__ import annotations

import re
import shutil
import subprocess
from pathlib import Path

from python_tools.ci.release_support import resolve_release_tag


class ReleaseSourcePackager:
    def resolve_tag(self, explicit: str | None) -> str:
        return resolve_release_tag(explicit)

    def package(self, repo_root: Path, dist_dir: Path, tag: str, ref: str = "HEAD") -> Path:
        if dist_dir.exists():
            shutil.rmtree(dist_dir)
        dist_dir.mkdir(parents=True, exist_ok=True)
        safe_tag = self._safe_filename(tag)
        archive = dist_dir / f"blitzar-{safe_tag}-source.zip"
        prefix = f"blitzar-{safe_tag}-source/"
        result = subprocess.run(
            ["git", "archive", "--format=zip", f"--prefix={prefix}", "-o", str(archive), ref],
            cwd=repo_root,
            check=False,
            capture_output=True,
            text=True,
        )
        if result.returncode != 0:
            detail = result.stderr.strip() or result.stdout.strip()
            raise RuntimeError(f"failed to package source archive: {detail}")
        return archive

    @staticmethod
    def _safe_filename(value: str) -> str:
        cleaned = re.sub(r"[^A-Za-z0-9_.-]+", "-", value.strip())
        return cleaned.strip(".-") or "manual"
