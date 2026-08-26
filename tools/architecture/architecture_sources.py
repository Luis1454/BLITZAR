"""CMake source completeness checks for the architecture gate."""

from __future__ import annotations

import json
import pathlib
import re

from tools.architecture.architecture_scope import COMPILED_SUFFIXES


CMAKE_SOURCE_PATTERN = re.compile(
    r"(?<![A-Za-z0-9_.-])((?:src|apps|examples|tests)/"
    r"[A-Za-z0-9_./\\-]+(?:\.cpp|\.cuh|\.cu|\.cc|\.hip|\.c)(?![A-Za-z0-9_]))"
)


def load_source_completeness(root: pathlib.Path) -> dict[str, object]:
    quality_path = root / "plan" / "quality.json"
    defaults = {
        "cmake_files": ["CMakeLists.txt"],
        "source_roots": ["src", "apps", "examples", "tests"],
        "suffixes": sorted(COMPILED_SUFFIXES),
    }
    if not quality_path.is_file():
        return defaults
    quality = json.loads(quality_path.read_text(encoding="utf-8"))
    architecture = quality.get("architecture", {})
    configured = architecture.get("source_completeness", {})
    if not isinstance(configured, dict):
        return defaults
    result = dict(defaults)
    for key in defaults:
        value = configured.get(key, defaults[key])
        if isinstance(value, list) and all(isinstance(item, str) for item in value):
            result[key] = value
    return result


def configured_paths(root: pathlib.Path, entries: object) -> list[pathlib.Path]:
    if not isinstance(entries, list):
        return []
    paths: list[pathlib.Path] = []
    for entry in entries:
        if not isinstance(entry, str):
            continue
        candidate = root / pathlib.PurePosixPath(entry.replace("\\", "/"))
        if candidate.is_dir():
            paths.extend(item for item in candidate.rglob("*") if item.is_file())
        elif candidate.is_file():
            paths.append(candidate)
    return paths


def source_completeness_report(root: pathlib.Path) -> dict[str, object]:
    configured = load_source_completeness(root)
    suffixes = {str(item).lower() for item in configured["suffixes"]}
    expected = {
        path.relative_to(root).as_posix()
        for path in configured_paths(root, configured["source_roots"])
        if path.suffix.lower() in suffixes
    }
    references: set[str] = set()
    cmake_files = configured_paths(root, configured["cmake_files"])
    for path in cmake_files:
        source = path.read_text(encoding="utf-8", errors="replace")
        references.update(
            item.replace("\\", "/") for item in CMAKE_SOURCE_PATTERN.findall(source)
        )
    missing = sorted(expected - references)
    stale = sorted(references - expected)
    return {
        "status": "ok" if not missing and not stale else "incomplete",
        "cmake_files": [path.relative_to(root).as_posix() for path in cmake_files],
        "expected_sources": len(expected),
        "referenced_sources": len(references),
        "missing": missing,
        "stale": stale,
    }
