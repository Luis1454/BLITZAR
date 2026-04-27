#!/usr/bin/env python3
# @file tests/checks/suites/support/path_specs.py
# @author Luis1454
# @project BLITZAR
# @brief Automated verification assets for BLITZAR quality gates.

from __future__ import annotations

from pathlib import Path

CPP_EXT = ".cpp"

ENGINE_SERVER_DIR = Path("engine") / "src" / "server"
ENGINE_CONFIG_DIR = Path("engine") / "src" / "config"
RUNTIME_SERVER_DIR = Path("runtime") / "src" / "server"
MODULES_QT_UI_DIR = Path("modules") / "qt" / "ui"
TESTS_UNIT_DIR = Path("tests") / "unit"


# @brief Documents the cpp file operation contract.
# @param base_dir Input value used by this contract.
# @param stem Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def cpp_file(base_dir: Path, stem: str) -> Path:
    return base_dir / f"{stem}{CPP_EXT}"
