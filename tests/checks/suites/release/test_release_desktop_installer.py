# @file tests/checks/suites/release/test_release_desktop_installer.py
# @author Luis1454
# @project BLITZAR
# @brief Automated verification assets for BLITZAR quality gates.

from __future__ import annotations

import subprocess
from pathlib import Path

from python_tools.ci import windows_installer as windows_installer_module
from python_tools.ci.release_bundle import ReleaseBundlePackager


# @brief Documents the test nsis installer does not use executable as wizard icon operation contract.
# @param None This contract does not take explicit parameters.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def test_nsis_installer_does_not_use_executable_as_wizard_icon() -> None:
    nsis_script = Path("scripts/install/windows/BLITZAR.nsi").read_text(encoding="utf-8")

    assert "!define MUI_ICON" not in nsis_script
    assert "!define MUI_UNICON" not in nsis_script
    assert "--wait-for-module" in nsis_script


# @brief Documents the test release desktop installer builds native windows installer operation contract.
# @param tmp_path Input value used by this contract.
# @param monkeypatch Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def test_release_desktop_installer_builds_native_windows_installer(tmp_path: Path, monkeypatch) -> None:
    monkeypatch.chdir(tmp_path)
    build_dir = tmp_path / "build"
    build_dir.mkdir()
    for name in ("blitzar.exe", "blitzar-server.exe", "blitzar-client.exe", "blitzarClientModuleQtInProc.dll"):
        (build_dir / name).write_text("binary\n", encoding="utf-8")
    (build_dir / "blitzarClientModuleQtInProc.dll.manifest").write_text("manifest\n", encoding="utf-8")
    (build_dir / "Qt6Core.dll").write_text("qt\n", encoding="utf-8")
    (build_dir / "platforms").mkdir()
    (build_dir / "platforms" / "qwindows.dll").write_text("plugin\n", encoding="utf-8")
    (tmp_path / "simulation.ini").write_text("config\n", encoding="utf-8")
    (tmp_path / "README.md").write_text("readme\n", encoding="utf-8")

    installer_output = tmp_path / "dist/desktop-installer" / "blitzar-rc-1-windows-desktop-installer.exe"

    # @brief Documents the fake run operation contract.
    # @param command Input value used by this contract.
    # @param cwd Input value used by this contract.
    # @param check Input value used by this contract.
    # @param capture_output Input value used by this contract.
    # @param text Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
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
