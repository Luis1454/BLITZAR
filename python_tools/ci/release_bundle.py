#!/usr/bin/env python3
from __future__ import annotations

import shutil
from pathlib import Path

from python_tools.ci.release_name import resolve_release_tag


class ReleaseBundlePackager:
    def resolve_tag(self, explicit: str | None) -> str:
        return resolve_release_tag(explicit)

    def package(self, build_dir: Path, dist_dir: Path, tag: str, tool_manifest: Path | None = None) -> Path:
        if dist_dir.exists():
            shutil.rmtree(dist_dir)
        dist_dir.mkdir(parents=True, exist_ok=True)
        self._copy_binaries(build_dir, dist_dir)
        self._copy_metadata(dist_dir, tool_manifest)
        archive_base = dist_dir / f"CUDA-GRAVITY-SIMULATION-{tag}-windows"
        return Path(shutil.make_archive(str(archive_base), "zip", root_dir=dist_dir))

    def _copy_binaries(self, build_dir: Path, dist_dir: Path) -> None:
        for name in ("myApp.exe", "myAppBackend.exe", "myAppHeadless.exe", "myAppModuleHost.exe"):
            src = build_dir / name
            if src.exists():
                shutil.copy2(src, dist_dir / name)

    def _copy_metadata(self, dist_dir: Path, tool_manifest: Path | None) -> None:
        for extra in ("simulation.ini", "README.md"):
            src = Path(extra)
            if src.exists():
                shutil.copy2(src, dist_dir / src.name)
        if tool_manifest is not None and tool_manifest.exists():
            shutil.copy2(tool_manifest, dist_dir / "tool_manifest.json")
