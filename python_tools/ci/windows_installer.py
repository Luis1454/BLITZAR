#!/usr/bin/env python3
# File: python_tools/ci/windows_installer.py
# Purpose: Python quality and automation support for BLITZAR governance.

from __future__ import annotations

import os
import shutil
import subprocess
from collections.abc import Callable
from pathlib import Path


class WindowsInstallerBuilder:
    def __init__(self, runner: Callable[..., subprocess.CompletedProcess[str]] | None = None) -> None:
        self._runner = runner if runner is not None else subprocess.run

    def build(self, source_dir: Path, output_dir: Path, tag: str) -> Path:
        source_dir = source_dir.resolve()
        output_dir = output_dir.resolve()
        output_dir.mkdir(parents=True, exist_ok=True)
        installer_path = output_dir / f"blitzar-{tag}-windows-desktop-installer.exe"
        script_path = self._script_path()
        makensis = self._locate_makensis()
        command = [
            str(makensis),
            f"/DINPUT_DIR={source_dir}",
            f"/DOUTPUT_FILE={installer_path}",
            f"/DPRODUCT_VERSION={tag}",
            str(script_path),
        ]
        completed = self._runner(command, cwd=output_dir, check=False, capture_output=True, text=True)
        if completed.returncode != 0:
            detail = (completed.stderr or completed.stdout or "").strip()
            raise RuntimeError(f"NSIS build failed for {installer_path.name}: {detail}")
        if not installer_path.exists():
            raise RuntimeError(f"NSIS did not produce installer: {installer_path}")
        return installer_path

    @staticmethod
    def _script_path() -> Path:
        return Path(__file__).resolve().parents[2] / "scripts" / "install" / "windows" / "BLITZAR.nsi"

    @staticmethod
    def _locate_makensis() -> Path:
        candidate = shutil.which("makensis") or shutil.which("makensis.exe")
        if candidate:
            return Path(candidate)
        search_roots = (
            Path(os.environ.get("PROGRAMFILES", r"C:\Program Files")),
            Path(os.environ.get("PROGRAMFILES(X86)", r"C:\Program Files (x86)")),
        )
        for root in search_roots:
            for relative in ("NSIS/makensis.exe", "NSIS/makensis"):
                candidate_path = root / relative
                if candidate_path.exists():
                    return candidate_path
        raise RuntimeError("NSIS makensis executable not found. Install NSIS to build the desktop installer.")