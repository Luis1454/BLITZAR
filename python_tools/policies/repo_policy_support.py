#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path

from python_tools.core.models import CheckResult
from python_tools.policies.repo_policy_power_of_10 import check_power_of_10_content

FORBIDDEN_CPP_EXTS = {".h", ".hh", ".hxx", ".c", ".cc", ".cxx"}
LINE_COUNT_EXTS = {".cmake", ".cpp", ".cu", ".cuh", ".h", ".hh", ".hpp", ".hxx", ".inl", ".ini", ".json", ".md", ".mk", ".ps1", ".py", ".sh", ".toml", ".txt", ".xml", ".yaml", ".yml"}
LINE_COUNT_NAMES = {"CMakeLists.txt", "Makefile"}
IGNORED_DIRS = {".git", ".idea", ".vs", "__pycache__", "build", "exports"}
HEADER_EXTS = {".hpp", ".h", ".hh", ".hxx"}
TEXT_EXTS = LINE_COUNT_EXTS | {".json", ".toml", ".xml"}
CPP_SCAN_EXTS = {".cpp", ".cc", ".cxx", ".c", ".cu", ".cuh", ".hpp", ".h", ".hh", ".hxx", ".inl"}
PROD_ROOTS = ("apps/", "engine/", "runtime/", "modules/")
EVIDENCE_WORKFLOW_PATHS = (".github/workflows/pr-fast-quality-gate.yml", ".github/workflows/nightly-full.yml", ".github/workflows/release-lane.yml")
LEGACY_CTEST_SELECTORS = ("ConfigArgsTest", "BackendProtocolTest", "FrontendBridgeTest", "FrontendRuntimeTest", "QtMainWindowTest")

FORBIDDEN_MARKER_RE = re.compile(r"(?m)(#|//|/\*)\s*(TODO|FIXME|HACK)\b")
GTEST_INCLUDE_RE = re.compile(r'#include\s*[<"]gtest/gtest\.h[>"]')
GTEST_MACRO_RE = re.compile(r"\bTEST(?:_F)?\s*\(")
UNNAMED_NAMESPACE_RE = re.compile(r"(?m)^\s*namespace\s*\{")
USING_ANY_RE = re.compile(r"(?m)^\s*using\b[^;]*;")
INLINE_NAMESPACE_RE = re.compile(r"(?m)^\s*namespace\s+[A-Za-z0-9_]+::[A-Za-z0-9_:]+\s*\{")
NAMESPACE_BLOCK_RE = re.compile(r"(?m)^\s*namespace\s+[A-Za-z0-9_]+\s*\{")
GRAVITY_INTERNAL_NAMESPACE_RE = re.compile(r"(?m)^\s*namespace\s+gravity_internal_")
QT_REFERENCE_NEW_RE = re.compile(r"(?m)^\s*(?:auto|Q[A-Za-z0-9_<>:]+)\s*&\s*[A-Za-z0-9_]+\s*=\s*\*new\s+Q[A-Za-z0-9_<>:]+\s*\(")
PREPROCESSOR_CONDITIONAL_RE = re.compile(r"(?m)^\s*#(?:if|ifdef|ifndef|elif|else|endif)\b")
PRAGMA_ONCE_RE = re.compile(r"(?m)^\s*#pragma\s+once\b")
DEFINE_RE = re.compile(r"(?m)^\s*#define\s+([A-Z][A-Z0-9_]+)\b(?!\s*\()")
COMMENT_ONLY_LINE_RE = re.compile(r"^\s*(?://|/\*|\*|\*/)")


def should_skip_dir(dirname: str) -> bool:
    return dirname in IGNORED_DIRS or dirname.startswith(("build-", "cmake-build-", ".pytest-basetemp"))


def should_count_lines(path: Path) -> bool:
    return path.name in LINE_COUNT_NAMES or path.suffix.lower() in LINE_COUNT_EXTS


def load_allowlist(path: Path) -> set[str]:
    if not path.exists():
        return set()
    entries: set[str] = set()
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if line and not line.startswith("#"):
            entries.add(line)
    return entries


def is_prod_path(rel: str) -> bool:
    return rel.startswith(PROD_ROOTS)


def check_cpp_content(rel: str, content: str, result: CheckResult) -> None:
    suffix = Path(rel).suffix.lower()
    if not rel.startswith("tests/"):
        if GTEST_INCLUDE_RE.search(content):
            result.add_error(f"{rel}: gtest include found outside tests/")
        if GTEST_MACRO_RE.search(content):
            result.add_error(f"{rel}: gtest TEST macro found outside tests/")
    if UNNAMED_NAMESPACE_RE.search(content):
        detail = "unnamed namespace is forbidden in production paths" if is_prod_path(rel) else "unnamed namespace is forbidden"
        result.add_error(f"{rel}: {detail}")
    if USING_ANY_RE.search(content):
        result.add_error(f"{rel}: 'using' is forbidden in C++ sources")
    if INLINE_NAMESPACE_RE.search(content):
        result.add_error(f"{rel}: nested namespace declaration (A::B) is forbidden")
    if GRAVITY_INTERNAL_NAMESPACE_RE.search(content):
        result.add_error(f"{rel}: gravity_internal_* namespace is forbidden")
    if is_prod_path(rel) and len(NAMESPACE_BLOCK_RE.findall(content)) > 1:
        result.add_error(f"{rel}: nested namespace blocks are forbidden in production paths")
    if PRAGMA_ONCE_RE.search(content):
        result.add_error(f"{rel}: #pragma once is forbidden; use include guards")
    if suffix in HEADER_EXTS:
        check_include_guard(rel, content, result)
    else:
        if PREPROCESSOR_CONDITIONAL_RE.search(content):
            result.add_error(f"{rel}: preprocessor conditionals are forbidden in C/C++ sources")
        if DEFINE_RE.search(content):
            result.add_error(f"{rel}: preprocessor macros are forbidden in C/C++ sources")
    if is_prod_path(rel):
        for error in check_power_of_10_content(rel, content):
            result.add_error(error)
    if rel.startswith("modules/qt/") and QT_REFERENCE_NEW_RE.search(content):
        result.add_error(f"{rel}: Qt '*new + reference' ownership pattern is forbidden")


def check_include_guard(rel: str, content: str, result: CheckResult) -> None:
    non_empty = [(index, line.strip()) for index, line in enumerate(content.splitlines()) if line.strip()]
    effective = _strip_leading_header_comments(non_empty)
    if len(effective) < 3:
        result.add_error(f"{rel}: header must use a strict include guard")
        return
    (_, first), (_, second), (_, last) = effective[0], effective[1], effective[-1]
    if not first.startswith("#ifndef "):
        result.add_error(f"{rel}: header must start with #ifndef include guard")
        return
    guard_name = first.split(maxsplit=1)[1]
    if second != f"#define {guard_name}":
        result.add_error(f"{rel}: header include guard must define the same symbol on the second line")
    if not last.startswith("#endif"):
        result.add_error(f"{rel}: header must end with #endif include guard")
    conditional_positions = [index for index, line in effective if PREPROCESSOR_CONDITIONAL_RE.match(line)]
    if conditional_positions != [effective[0][0], effective[-1][0]]:
        result.add_error(f"{rel}: header must not use preprocessor conditionals beyond the include guard")
    define_positions = [(index, name) for index, line in effective for name in DEFINE_RE.findall(line)]
    if define_positions != [(effective[1][0], guard_name)]:
        result.add_error(f"{rel}: header must not define macros beyond the include guard")


def _strip_leading_header_comments(non_empty: list[tuple[int, str]]) -> list[tuple[int, str]]:
    start = 0
    while start < len(non_empty) and COMMENT_ONLY_LINE_RE.match(non_empty[start][1]):
        start += 1
    return non_empty[start:]


def collect_marker_commands(content: str, marker_text: str) -> list[str]:
    commands: list[str] = []
    lines = content.splitlines()
    index = 0
    while index < len(lines):
        stripped = lines[index].strip()
        marker = stripped.find(marker_text)
        if marker == -1:
            index += 1
            continue
        command = stripped[marker:]
        while command.endswith("\\") and index + 1 < len(lines):
            index += 1
            command = f"{command} {lines[index].strip()}"
        commands.append(command)
        index += 1
    return commands
