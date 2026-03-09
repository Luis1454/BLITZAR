#!/usr/bin/env python3
from __future__ import annotations

import zipfile
from pathlib import Path

from python_tools.release.release_bundle import ReleaseBundlePackager


def test_release_bundle_embeds_tool_manifest(tmp_path: Path, monkeypatch) -> None:
    monkeypatch.chdir(tmp_path)
    build_dir = tmp_path / "build"
    build_dir.mkdir()
    (build_dir / "myApp.exe").write_text("binary\n", encoding="utf-8")
    (tmp_path / "simulation.ini").write_text("config\n", encoding="utf-8")
    (tmp_path / "README.md").write_text("readme\n", encoding="utf-8")
    tool_manifest = tmp_path / "dist/tool-qualification/tool_manifest.json"
    tool_manifest.parent.mkdir(parents=True, exist_ok=True)
    tool_manifest.write_text('{"lane":"release"}\n', encoding="utf-8")

    archive = ReleaseBundlePackager().package(build_dir, tmp_path / "dist/release-bundle", "rc-1", tool_manifest)

    with zipfile.ZipFile(archive) as bundle:
        names = bundle.namelist()
        assert "tool_manifest.json" in names
        assert "myApp.exe" in names
