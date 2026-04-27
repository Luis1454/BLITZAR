#!/usr/bin/env python3
# File: tests/checks/suites/support/path_specs.py
# Purpose: Verification coverage for the BLITZAR quality gate.

from __future__ import annotations

from pathlib import Path

CPP_EXT = ".cpp"

ENGINE_SERVER_DIR = Path("engine") / "src" / "server"
ENGINE_CONFIG_DIR = Path("engine") / "src" / "config"
RUNTIME_SERVER_DIR = Path("runtime") / "src" / "server"
MODULES_QT_UI_DIR = Path("modules") / "qt" / "ui"
TESTS_UNIT_DIR = Path("tests") / "unit"


# Description: Executes the cpp_file operation.
def cpp_file(base_dir: Path, stem: str) -> Path:
    return base_dir / f"{stem}{CPP_EXT}"
