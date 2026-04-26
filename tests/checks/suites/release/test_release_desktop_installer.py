from __future__ import annotations

import zipfile
from pathlib import Path

from python_tools.ci.release_bundle import ReleaseBundlePackager


def test_release_desktop_installer_embeds_gui_launcher_and_install_scripts(tmp_path: Path, monkeypatch) -> None:
    monkeypatch.chdir(tmp_path)
    build_dir = tmp_path / "build"
    build_dir.mkdir()
    for name in ("blitzar.exe", "blitzar-server.exe", "blitzar-client.exe", "gravityClientModuleQtInProc.dll"):
        (build_dir / name).write_text("binary\n", encoding="utf-8")
    (build_dir / "gravityClientModuleQtInProc.dll.manifest").write_text("manifest\n", encoding="utf-8")
    (build_dir / "Qt6Core.dll").write_text("qt\n", encoding="utf-8")
    (build_dir / "platforms").mkdir()
    (build_dir / "platforms" / "qwindows.dll").write_text("plugin\n", encoding="utf-8")
    (tmp_path / "simulation.ini").write_text("config\n", encoding="utf-8")
    (tmp_path / "README.md").write_text("readme\n", encoding="utf-8")

    archive = ReleaseBundlePackager().package(
        build_dir,
        tmp_path / "dist/desktop-installer",
        "rc-1",
        artifact_kind="desktop-installer",
    )

    assert archive.name == "blitzar-rc-1-windows-desktop-installer.zip"
    with zipfile.ZipFile(archive) as bundle:
        names = bundle.namelist()
    assert "Launch BLITZAR GUI.cmd" in names
    assert "Install BLITZAR.cmd" in names
    assert "Install-BLITZAR.ps1" in names
    assert "Uninstall BLITZAR.cmd" in names
    assert "Uninstall-BLITZAR.ps1" in names
    assert "gravityClientModuleQtInProc.dll" in names
    assert "platforms/qwindows.dll" in names
