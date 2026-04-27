#!/usr/bin/env python3
# @file scripts/ci/release/smoke_windows_installer.py
# @author Luis1454
# @project BLITZAR
# @brief Build, release, and CI helper automation for BLITZAR workflows.

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))


# @brief Documents the locate 7z operation contract.
# @param None This contract does not take explicit parameters.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def _locate_7z() -> Path:
    candidate = shutil.which("7z") or shutil.which("7z.exe")
    if candidate:
        return Path(candidate)
    program_files = (
        Path(os.environ.get("PROGRAMFILES", r"C:\Program Files")),
        Path(os.environ.get("PROGRAMFILES(X86)", r"C:\Program Files (x86)")),
    )
    for root in program_files:
        candidate_path = root / "7-Zip" / "7z.exe"
        if candidate_path.exists():
            return candidate_path
    raise RuntimeError("7z executable not found. Install 7-Zip to validate the installer archive.")


# @brief Documents the parse args operation contract.
# @param None This contract does not take explicit parameters.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Validate a packaged native Windows installer.")
    parser.add_argument("--installer", required=True, help="Installer executable produced by package_bundle.py")
    parser.add_argument("--extract-dir", default="", help="Directory used to extract the installer contents")
    return parser.parse_args()


# @brief Documents the main operation contract.
# @param None This contract does not take explicit parameters.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def main() -> int:
    args = parse_args()
    installer = Path(args.installer)
    extract_dir = Path(args.extract_dir) if args.extract_dir.strip() else installer.with_suffix("")
    if extract_dir.exists():
        shutil.rmtree(extract_dir)
    extract_dir.mkdir(parents=True, exist_ok=True)

    seven_zip = _locate_7z()
    completed = subprocess.run(
        [str(seven_zip), "x", "-y", f"-o{extract_dir}", str(installer)],
        check=False,
        capture_output=True,
        text=True,
    )
    if completed.returncode != 0:
        detail = completed.stderr.strip() or completed.stdout.strip()
        raise RuntimeError(f"installer extraction failed: {detail}")

    required = (
        "blitzar.exe",
        "blitzar-client.exe",
        "gravityClientModuleQtInProc.dll",
        "gravityClientModuleQtInProc.dll.manifest",
        "simulation.ini",
        "README.md",
    )
    missing = [name for name in required if not (extract_dir / name).exists()]
    if missing:
        raise RuntimeError(f"installer missing expected files after extraction: {', '.join(missing)}")

    print(extract_dir.as_posix())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
