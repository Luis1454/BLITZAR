#!/usr/bin/env python3
from __future__ import annotations

import shutil
import subprocess
import zipfile
from collections.abc import Callable
from pathlib import Path

from python_tools.ci.release_support import resolve_release_tag


class ReleaseBundlePackager:
    _EXECUTABLES = (
        "blitzar.exe",
        "blitzar-server.exe",
        "blitzar-headless.exe",
        "blitzar-client.exe",
    )
    _RUNTIME_DIRECTORIES = (
        "generic",
        "iconengines",
        "imageformats",
        "networkinformation",
        "platforms",
        "resources",
        "styles",
        "tls",
    )
    _ROOT_RUNTIME_FILES = (
        "qt.conf",
        "vc_redist.x64.exe",
    )
    _DESKTOP_INSTALLER_FILES = (
        "Launch BLITZAR GUI.cmd",
        "Install BLITZAR.cmd",
        "Install-BLITZAR.ps1",
        "Uninstall BLITZAR.cmd",
        "Uninstall-BLITZAR.ps1",
    )

    def resolve_tag(self, explicit: str | None) -> str:
        return resolve_release_tag(explicit)

    def package(
        self,
        build_dir: Path,
        dist_dir: Path,
        tag: str,
        tool_manifest: Path | None = None,
        artifact_kind: str = "portable",
    ) -> Path:
        if dist_dir.exists():
            shutil.rmtree(dist_dir)
        dist_dir.mkdir(parents=True, exist_ok=True)
        self._copy_binaries(build_dir, dist_dir)
        self._copy_metadata(dist_dir, tool_manifest)
        if artifact_kind == "desktop-installer":
            self._require_desktop_gui(dist_dir)
            self._copy_desktop_installer(dist_dir)
        archive_suffix = "windows-desktop-installer" if artifact_kind == "desktop-installer" else "windows"
        archive_base = dist_dir.parent / f"blitzar-{tag}-{archive_suffix}"
        archive_path = Path(shutil.make_archive(str(archive_base), "zip", root_dir=dist_dir))
        final_archive = dist_dir / archive_path.name
        if final_archive.exists():
            final_archive.unlink()
        shutil.move(str(archive_path), str(final_archive))
        return final_archive

    def _copy_binaries(self, build_dir: Path, dist_dir: Path) -> None:
        for name in self._EXECUTABLES:
            src = build_dir / name
            if src.exists():
                shutil.copy2(src, dist_dir / name)
        for src in sorted(build_dir.glob("*.dll")):
            shutil.copy2(src, dist_dir / src.name)
        for src in sorted(build_dir.glob("*.dll.manifest")):
            shutil.copy2(src, dist_dir / src.name)
        for name in self._ROOT_RUNTIME_FILES:
            src = build_dir / name
            if src.exists():
                shutil.copy2(src, dist_dir / src.name)
        for name in self._RUNTIME_DIRECTORIES:
            src = build_dir / name
            if src.exists() and src.is_dir():
                shutil.copytree(src, dist_dir / name, dirs_exist_ok=True)

    def _copy_metadata(self, dist_dir: Path, tool_manifest: Path | None) -> None:
        for extra in ("simulation.ini", "README.md"):
            src = Path(extra)
            if src.exists():
                shutil.copy2(src, dist_dir / src.name)
        if tool_manifest is not None and tool_manifest.exists():
            shutil.copy2(tool_manifest, dist_dir / "tool_manifest.json")

    def _copy_desktop_installer(self, dist_dir: Path) -> None:
        source_root = Path(__file__).resolve().parents[2] / "scripts" / "install" / "windows"
        for name in self._DESKTOP_INSTALLER_FILES:
            src = source_root / name
            if not src.exists():
                raise RuntimeError(f"missing desktop installer file: {src}")
            shutil.copy2(src, dist_dir / name)

    @staticmethod
    def _require_desktop_gui(dist_dir: Path) -> None:
        required = (
            "blitzar.exe",
            "blitzar-client.exe",
            "blitzar-server.exe",
            "gravityClientModuleQtInProc.dll",
            "gravityClientModuleQtInProc.dll.manifest",
        )
        missing = [name for name in required if not (dist_dir / name).exists()]
        if missing:
            raise RuntimeError(f"desktop installer missing GUI runtime files: {', '.join(missing)}")


class ReleaseBundleSmokeValidator:
    _HELP_COMMANDS = (
        ("blitzar.exe", "--help"),
        ("blitzar-headless.exe", "--help"),
        ("blitzar-server.exe", "--server-help"),
        ("blitzar-client.exe", "--help"),
    )

    def __init__(self, runner: Callable[[list[str], Path], None] | None = None) -> None:
        self._runner = runner if runner is not None else self._run_command

    def validate_archive(self, archive: Path, extract_dir: Path | None = None, run_commands: bool = True) -> Path:
        bundle_root = extract_dir if extract_dir is not None else archive.with_suffix("")
        if bundle_root.exists():
            shutil.rmtree(bundle_root)
        bundle_root.mkdir(parents=True, exist_ok=True)
        with zipfile.ZipFile(archive) as bundle:
            bundle.extractall(bundle_root)
        self.validate_layout(bundle_root)
        if run_commands:
            self.run_smoke(bundle_root)
        return bundle_root

    def validate_layout(self, bundle_root: Path) -> None:
        required_files = ("simulation.ini", "README.md")
        if not any((bundle_root / name).exists() for name, *_ in self._HELP_COMMANDS):
            raise RuntimeError("portable bundle does not contain any runnable BLITZAR executable")
        for name in required_files:
            if not (bundle_root / name).exists():
                raise RuntimeError(f"portable bundle missing required file: {name}")
        self._validate_module_manifests(bundle_root)
        self._validate_qt_runtime(bundle_root)

    def run_smoke(self, bundle_root: Path) -> None:
        for program, *args in self._HELP_COMMANDS:
            exe = bundle_root / program
            if exe.exists():
                self._runner([str(exe), *args], bundle_root)

    @staticmethod
    def _run_command(command: list[str], cwd: Path) -> None:
        completed = subprocess.run(
            command,
            cwd=cwd,
            check=False,
            capture_output=True,
            text=True,
        )
        if completed.returncode != 0:
            detail = completed.stderr.strip() or completed.stdout.strip()
            raise RuntimeError(f"portable smoke command failed ({' '.join(command)}): {detail}")

    @staticmethod
    def _validate_module_manifests(bundle_root: Path) -> None:
        for module_path in sorted(bundle_root.glob("gravityClientModule*.dll")):
            manifest_path = bundle_root / f"{module_path.name}.manifest"
            if not manifest_path.exists():
                raise RuntimeError(f"portable bundle missing module manifest for {module_path.name}")

    @staticmethod
    def _validate_qt_runtime(bundle_root: Path) -> None:
        has_qt_module = (bundle_root / "gravityClientModuleQtInProc.dll").exists()
        has_qt_runtime = any((bundle_root / dll).exists() for dll in ("Qt6Core.dll", "Qt6Cored.dll"))
        if has_qt_module or has_qt_runtime:
            plugin_path = bundle_root / "platforms" / "qwindows.dll"
            debug_plugin_path = bundle_root / "platforms" / "qwindowsd.dll"
            if not plugin_path.exists() and not debug_plugin_path.exists():
                raise RuntimeError("portable bundle missing Qt platform plugin under platforms/")
