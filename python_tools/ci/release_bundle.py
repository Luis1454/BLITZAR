#!/usr/bin/env python3
from __future__ import annotations

import os
import shutil
from pathlib import Path


class ReleaseBundlePackager:
    def resolve_tag(self, explicit: str | None) -> str:
        if explicit and explicit.strip():
            return explicit.strip()
        ref_name = os.environ.get("GITHUB_REF_NAME", "").strip()
        if ref_name:
            return ref_name
        run_number = os.environ.get("GITHUB_RUN_NUMBER", "").strip()
        if run_number:
            return f"manual-{run_number}"
        return "manual"

    def package(self, build_dir: Path, dist_dir: Path, tag: str) -> Path:
        if dist_dir.exists():
            shutil.rmtree(dist_dir)
        dist_dir.mkdir(parents=True, exist_ok=True)
        self._copy_binaries(build_dir, dist_dir)
        self._copy_metadata(dist_dir)
        archive_base = dist_dir / f"CUDA-GRAVITY-SIMULATION-{tag}-windows"
        return Path(shutil.make_archive(str(archive_base), "zip", root_dir=dist_dir))

    def _copy_binaries(self, build_dir: Path, dist_dir: Path) -> None:
        for name in ("myApp.exe", "myAppBackend.exe", "myAppHeadless.exe", "myAppModuleHost.exe"):
            src = build_dir / name
            if src.exists():
                shutil.copy2(src, dist_dir / name)

    def _copy_metadata(self, dist_dir: Path) -> None:
        for extra in ("simulation.ini", "README.md"):
            src = Path(extra)
            if src.exists():
                shutil.copy2(src, dist_dir / src.name)

