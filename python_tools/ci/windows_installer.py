#!/usr/bin/env python3
# @file python_tools/ci/windows_installer.py
# @author Luis1454
# @project BLITZAR
# @brief Python quality and automation support for BLITZAR governance.

from __future__ import annotations

import os
import shutil
import subprocess
from collections.abc import Callable
from pathlib import Path


# @brief Defines the windows installer builder type contract.
# @param None This contract does not take explicit parameters.
# @note Keep construction and side effects explicit for deterministic quality gates.
class WindowsInstallerBuilder:
    # @brief Documents the init operation contract.
    # @param runner Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def __init__(self, runner: Callable[..., subprocess.CompletedProcess[str]] | None = None) -> None:
        self._runner = runner if runner is not None else subprocess.run

    # @brief Documents the build operation contract.
    # @param source_dir Input value used by this contract.
    # @param output_dir Input value used by this contract.
    # @param tag Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
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
    # @brief Documents the script path operation contract.
    # @param None This contract does not take explicit parameters.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _script_path() -> Path:
        return Path(__file__).resolve().parents[2] / "scripts" / "install" / "windows" / "BLITZAR.nsi"

    @staticmethod
    # @brief Documents the locate makensis operation contract.
    # @param None This contract does not take explicit parameters.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
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
