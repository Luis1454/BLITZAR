#!/usr/bin/env python3
"""Inventory generated local outputs without modifying the workspace."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Any

GENERATED_PREFIXES = (
    "build",
    "cmake-build-",
    "dist",
    "artifacts",
    "exports",
    "outputs",
    ".pytest-basetemp",
    ".pytest_cache",
    ".mypy_cache",
    ".ruff_cache",
)
BINARY_SUFFIXES = {
    ".a",
    ".bin",
    ".dll",
    ".dylib",
    ".exe",
    ".lib",
    ".o",
    ".obj",
    ".pdb",
    ".so",
    ".wasm",
    ".zip",
}


@dataclass(frozen=True)
class LocalArtifactRoot:
    path: str
    file_count: int
    binary_count: int
    total_bytes: int
    newest_mtime: float | None
    oldest_mtime: float | None


def is_generated_root(path: Path) -> bool:
    return path.name.startswith(GENERATED_PREFIXES)


def scan_root(path: Path) -> LocalArtifactRoot:
    file_count = 0
    binary_count = 0
    total_bytes = 0
    newest_mtime: float | None = None
    oldest_mtime: float | None = None
    for item in path.rglob("*"):
        if not item.is_file():
            continue
        stat = item.stat()
        file_count += 1
        total_bytes += stat.st_size
        binary_count += int(item.suffix.lower() in BINARY_SUFFIXES)
        newest_mtime = stat.st_mtime if newest_mtime is None else max(newest_mtime, stat.st_mtime)
        oldest_mtime = stat.st_mtime if oldest_mtime is None else min(oldest_mtime, stat.st_mtime)
    return LocalArtifactRoot(
        path=str(path),
        file_count=file_count,
        binary_count=binary_count,
        total_bytes=total_bytes,
        newest_mtime=newest_mtime,
        oldest_mtime=oldest_mtime,
    )


def build_report(root: Path) -> dict[str, Any]:
    roots = [item for item in root.iterdir() if item.is_dir() and is_generated_root(item)]
    entries = [scan_root(item) for item in sorted(roots)]
    return {
        "root": str(root),
        "roots": [entry.__dict__ for entry in entries],
        "total_file_count": sum(entry.file_count for entry in entries),
        "total_binary_count": sum(entry.binary_count for entry in entries),
        "total_bytes": sum(entry.total_bytes for entry in entries),
    }
