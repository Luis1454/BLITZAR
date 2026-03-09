#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

CPP_EXT = ".cpp"

ENGINE_BACKEND_DIR = Path("engine") / "src" / "backend"
ENGINE_CONFIG_DIR = Path("engine") / "src" / "config"
RUNTIME_BACKEND_DIR = Path("runtime") / "src" / "backend"
MODULES_QT_UI_DIR = Path("modules") / "qt" / "ui"
TESTS_UNIT_DIR = Path("tests") / "unit"


def cpp_file(base_dir: Path, stem: str) -> Path:
    return base_dir / f"{stem}{CPP_EXT}"
