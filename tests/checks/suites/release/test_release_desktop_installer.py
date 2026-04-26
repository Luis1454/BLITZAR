from __future__ import annotations

import subprocess
from pathlib import Path

from python_tools.ci import windows_installer as windows_installer_module
from python_tools.ci.release_bundle import ReleaseBundlePackager


def test_release_desktop_installer_builds_native_windows_installer(tmp_path: Path, monkeypatch) -> None:
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

    installer_output = tmp_path / "dist/desktop-installer" / "blitzar-rc-1-windows-desktop-installer.exe"

    def fake_run(command: list[str], cwd: Path, check: bool, capture_output: bool, text: bool):
        del cwd, check, capture_output, text
        for entry in command:
            if entry.startswith("/DOUTPUT_FILE="):
                Path(entry.split("=", 1)[1]).write_bytes(b"installer\n")
                break
        return subprocess.CompletedProcess(command, 0, stdout="", stderr="")

    monkeypatch.setattr(windows_installer_module.shutil, "which", lambda name: "C:/NSIS/makensis.exe" if name.startswith("makensis") else None)
    monkeypatch.setattr(windows_installer_module.subprocess, "run", fake_run)

    installer = ReleaseBundlePackager().package(
        build_dir,
        tmp_path / "dist/desktop-installer",
        "rc-1",
        artifact_kind="desktop-installer",
    )

    assert installer == installer_output
    assert installer.name == "blitzar-rc-1-windows-desktop-installer.exe"
    assert installer.exists()
    assert not (installer.parent / "stage").exists()
