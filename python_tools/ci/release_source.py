#!/usr/bin/env python3
# @file python_tools/ci/release_source.py
# @author Luis1454
# @project BLITZAR
# @brief Python quality and automation support for BLITZAR governance.

from __future__ import annotations

import re
import shutil
import subprocess
from pathlib import Path

from python_tools.ci.release_support import resolve_release_tag


# @brief Defines the release source packager type contract.
# @param None This contract does not take explicit parameters.
# @note Keep construction and side effects explicit for deterministic quality gates.
class ReleaseSourcePackager:
    # @brief Documents the resolve tag operation contract.
    # @param explicit Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def resolve_tag(self, explicit: str | None) -> str:
        return resolve_release_tag(explicit)

    # @brief Documents the package operation contract.
    # @param repo_root Input value used by this contract.
    # @param dist_dir Input value used by this contract.
    # @param tag Input value used by this contract.
    # @param ref Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
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
    # @brief Documents the safe filename operation contract.
    # @param value Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _safe_filename(value: str) -> str:
        cleaned = re.sub(r"[^A-Za-z0-9_.-]+", "-", value.strip())
        return cleaned.strip(".-") or "manual"
