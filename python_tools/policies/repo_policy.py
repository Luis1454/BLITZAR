#!/usr/bin/env python3
# File: python_tools/policies/repo_policy.py
# Purpose: Python quality and automation support for BLITZAR governance.

from __future__ import annotations

import re
from pathlib import Path

from python_tools.core.base_check import BaseCheck
from python_tools.core.models import CheckContext, CheckResult
from python_tools.policies.header_definition_policy import find_header_function_definition_lines
from python_tools.policies.repo_policy_function_metrics import (
    IMPLEMENTATION_SCAN_EXTS,
    collect_function_decomposition_warnings,
)
from python_tools.policies.repo_policy_workflows import (
    check_evidence_workflow_commands,
    check_legacy_ctest_selectors,
    check_release_lane_subset,
    check_workflow_action_pinning,
    check_workflow_failure_masking,
    check_workflow_pip_manifest_usage,
)

FORBIDDEN_CPP_EXTS = {".h", ".hh", ".hxx", ".c", ".cc", ".cxx"}
LINE_COUNT_EXTS = {
    ".cmake", ".cpp", ".cu", ".cuh", ".h", ".hh", ".hpp", ".hxx", ".inl", ".ini", ".json",
    ".md", ".mk", ".ps1", ".py", ".sh", ".toml", ".txt", ".xml", ".yaml", ".yml",
}
LINE_COUNT_NAMES = {"CMakeLists.txt", "Makefile"}
IGNORED_DIRS = {
    ".git",
    ".idea",
    ".vs",
    "__pycache__",
    ".mypy_cache",
    ".ruff_cache",
    ".pytest_cache",
    "build",
    "exports",
    "site-packages",
}
FORBIDDEN_MARKER_RE = re.compile(r"(?m)(#|//|/\*)\s*(TODO|FIXME|HACK)\b")
HEADER_EXTS = {".hpp", ".h", ".hh", ".hxx"}
TEXT_EXTS = LINE_COUNT_EXTS | {".json", ".toml", ".xml"}
CPP_SCAN_EXTS = {".cpp", ".cc", ".cxx", ".c", ".cu", ".cuh", ".hpp", ".h", ".hh", ".hxx", ".inl"}
GTEST_INCLUDE_RE = re.compile(r'#include\s*[<"]gtest/gtest\.h[>"]')
GTEST_MACRO_RE = re.compile(r"\bTEST(?:_F)?\s*\(")
UNNAMED_NAMESPACE_RE = re.compile(r"(?m)^\s*namespace\s*\{")
USING_ANY_RE = re.compile(r"(?m)^\s*using\b[^;]*;")
INLINE_NAMESPACE_RE = re.compile(r"(?m)^\s*namespace\s+[A-Za-z0-9_]+::[A-Za-z0-9_:]+\s*\{")
NAMESPACE_BLOCK_RE = re.compile(r"(?m)^\s*namespace\s+[A-Za-z0-9_]+\s*\{")
GRAVITY_INTERNAL_NAMESPACE_RE = re.compile(r"(?m)^\s*namespace\s+gravity_internal_")
PROD_ROOTS = ("apps/", "engine/", "runtime/", "modules/")
GOTO_RE = re.compile(r"\bgoto\b")
SETJMP_LONGJMP_RE = re.compile(r"\b(?:setjmp|longjmp)\b")
DO_WHILE_RE = re.compile(r"\bdo\b\s*\{", re.DOTALL)
WHILE_TRUE_RE = re.compile(r"\bwhile\s*\(\s*true\s*\)")
FUNCTION_POINTER_TYPEDEF_RE = re.compile(r"(?m)^\s*(?:typedef|using)\b[^\n;]*\(\s*\*\s*[A-Za-z0-9_]*\s*\)")
FUNCTION_POINTER_ABI_PATHS = {"runtime/include/client/ClientModuleApi.hpp"}
NON_WAIVABLE_STRONG_SIZE_PATHS = {"engine/src/server/SimulationServer.cpp"}
QT_REFERENCE_NEW_RE = re.compile(
    r"(?m)^\s*(?:auto|Q[A-Za-z0-9_<>:]+)\s*&\s*[A-Za-z0-9_]+\s*=\s*\*new\s+Q[A-Za-z0-9_<>:]+\s*\("
)
PREPROCESSOR_CONDITIONAL_RE = re.compile(r"(?m)^\s*#(?:if|ifdef|ifndef|elif|else|endif)\b")
PRAGMA_ONCE_RE = re.compile(r"(?m)^\s*#pragma\s+once\b")
DEFINE_RE = re.compile(r"(?m)^\s*#define\s+([A-Z][A-Z0-9_]+)\b(?!\s*\()")

def should_skip_dir(dirname: str) -> bool:
    if dirname in IGNORED_DIRS:
        return True
    return dirname.startswith(("build-", "cmake-build-", ".pytest-basetemp", ".venv", ".tox"))


def should_skip_rel_parts(rel_parts: tuple[str, ...]) -> bool:
    if any(should_skip_dir(part) for part in rel_parts):
        return True
    for index, part in enumerate(rel_parts[:-1]):
        if part == "rust" and rel_parts[index + 1] == "target":
            return True
    return False


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


class RepoPolicyCheck(BaseCheck):
    name = "repo"
    success_message = "Repository policy check passed"
    failure_title = "Repository policy check failed:"
    warning_title = "Repository policy warnings:"

    def _execute(self, context: CheckContext, result: CheckResult) -> None:
        allowlist_path = context.allowlist if context.allowlist is not None else context.root / "tests/checks/policy_allowlist.txt"
        allowlist = load_allowlist(allowlist_path)
        used_allowlist: set[str] = set()

        for path in context.root.rglob("*"):
            if path.is_dir():
                continue
            rel_parts = path.relative_to(context.root).parts
            if should_skip_rel_parts(rel_parts):
                continue

            rel = path.relative_to(context.root).as_posix()
            suffix = path.suffix.lower()
            content: str | None = None
            if suffix in FORBIDDEN_CPP_EXTS:
                result.add_error(f"{rel}: forbidden C/C++ extension '{suffix}' (use .hpp/.cpp)")

            if suffix in TEXT_EXTS or path.name in LINE_COUNT_NAMES:
                content = path.read_text(encoding="utf-8", errors="ignore")
                if suffix in HEADER_EXTS and USING_ANY_RE.search(content):
                    result.add_error(f"{rel}: forbidden 'using' in header")
                if suffix in HEADER_EXTS:
                    for line_number in find_header_function_definition_lines(content):
                        result.add_error(f"{rel}:{line_number}: function definitions in headers are forbidden")
                marker_match = FORBIDDEN_MARKER_RE.search(content)
                if marker_match:
                    result.add_error(f"{rel}: forbidden marker '{marker_match.group(2)}' found")
                if suffix in CPP_SCAN_EXTS:
                    self._check_cpp_content(rel, content, result)

            if not should_count_lines(path):
                continue
            if content is None:
                content = path.read_text(encoding="utf-8", errors="ignore")
            self._check_line_count(rel, content, context, allowlist, used_allowlist, result)
            if suffix in IMPLEMENTATION_SCAN_EXTS:
                self._check_function_decomposition(rel, content, result)

        for stale in sorted(allowlist - used_allowlist):
            result.add_warning(f"allowlist entry not needed anymore: {stale}")

        check_evidence_workflow_commands(context.root, result, "cmake -S", "-DGRAVITY_PROFILE=prod", "evidence configure command must include -DGRAVITY_PROFILE=prod")
        check_evidence_workflow_commands(context.root, result, "ctest ", "--no-tests=error", "CI ctest command must include --no-tests=error")
        check_legacy_ctest_selectors(context.root, result)
        check_workflow_action_pinning(context.root, result)
        check_workflow_failure_masking(context.root, result)
        check_workflow_pip_manifest_usage(context.root, result)
        check_release_lane_subset(context.root, result)

    def _check_cpp_content(self, rel: str, content: str, result: CheckResult) -> None:
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
            self._check_include_guard(rel, content, result)
        else:
            if PREPROCESSOR_CONDITIONAL_RE.search(content):
                result.add_error(f"{rel}: preprocessor conditionals are forbidden in C/C++ sources")
            if DEFINE_RE.search(content):
                result.add_error(f"{rel}: preprocessor macros are forbidden in C/C++ sources")
        if is_prod_path(rel):
            for error in _check_power_of_10_content(rel, content):
                result.add_error(error)
        if rel.startswith("modules/qt/") and QT_REFERENCE_NEW_RE.search(content):
            result.add_error(f"{rel}: Qt '*new + reference' ownership pattern is forbidden")

    def _check_include_guard(self, rel: str, content: str, result: CheckResult) -> None:
        lines = content.splitlines()
        non_empty = [(index, line.strip()) for index, line in enumerate(lines) if line.strip()]
        if len(non_empty) < 3:
            result.add_error(f"{rel}: header must use a strict include guard")
            return

        guard_start = 0
        while guard_start < len(non_empty) and self._is_leading_header_comment(non_empty[guard_start][1]):
            guard_start += 1
        if len(non_empty) - guard_start < 3:
            result.add_error(f"{rel}: header must use a strict include guard")
            return

        (_, first), (_, second), (_, last) = non_empty[guard_start], non_empty[guard_start + 1], non_empty[-1]
        if not first.startswith("#ifndef "):
            result.add_error(f"{rel}: header must start with #ifndef include guard")
            return
        guard_name = first.split(maxsplit=1)[1]
        if second != f"#define {guard_name}":
            result.add_error(f"{rel}: header include guard must define the same symbol on the second line")
        if not last.startswith("#endif"):
            result.add_error(f"{rel}: header must end with #endif include guard")

        conditional_positions = [index for index, line in non_empty[guard_start:] if PREPROCESSOR_CONDITIONAL_RE.match(line)]
        if conditional_positions != [non_empty[guard_start][0], non_empty[-1][0]]:
            result.add_error(f"{rel}: header must not use preprocessor conditionals beyond the include guard")

        define_positions = [(index, name) for index, line in non_empty[guard_start:] for name in DEFINE_RE.findall(line)]
        if define_positions != [(non_empty[guard_start + 1][0], guard_name)]:
            result.add_error(f"{rel}: header must not define macros beyond the include guard")

    @staticmethod
    def _is_leading_header_comment(stripped: str) -> bool:
        return stripped.startswith(("//", "/*", "*"))

    def _check_line_count(
        self,
        rel: str,
        content: str,
        context: CheckContext,
        allowlist: set[str],
        used_allowlist: set[str],
        result: CheckResult,
    ) -> None:
        line_count = len(content.splitlines())
        if line_count > context.hard_lines:
            if rel in NON_WAIVABLE_STRONG_SIZE_PATHS:
                result.add_error(
                    f"{rel}: {line_count} lines exceeds non-waivable strong alert threshold {context.hard_lines} "
                    "(split by responsibility; deviation/allowlist is not permitted for this file)"
                )
                return
            if rel in allowlist:
                used_allowlist.add(rel)
                result.add_warning(
                    f"{rel}: strong file-size alert; {line_count} lines exceeds {context.hard_lines} "
                    "under an explicit deviation, keep the file coherent until it is split"
                )
            else:
                result.add_error(
                    f"{rel}: {line_count} lines exceeds strong alert threshold {context.hard_lines} "
                    "(split file or document a coherent exception in the deviation register)"
                )
            return
        if line_count > context.target_lines:
            result.add_warning(
                f"{rel}: {line_count} lines exceeds target {context.target_lines}; "
                "keep one primary responsibility and avoid artificial wrapper splits"
            )

    def _check_function_decomposition(self, rel: str, content: str, result: CheckResult) -> None:
        for warning in collect_function_decomposition_warnings(rel, content):
            result.add_warning(warning)


def _check_power_of_10_content(rel: str, content: str) -> list[str]:
    errors: list[str] = []
    if GOTO_RE.search(content):
        errors.append(f"{rel}: Power of 10 rule 1 forbids goto in production paths")
    if SETJMP_LONGJMP_RE.search(content):
        errors.append(f"{rel}: Power of 10 rule 1 forbids setjmp/longjmp in production paths")
    if DO_WHILE_RE.search(content):
        errors.append(f"{rel}: Power of 10 rule 1 forbids do-while in production paths")
    if WHILE_TRUE_RE.search(content):
        errors.append(f"{rel}: Power of 10 rule 2 forbids open-ended while(true) loops in production paths")
    if rel not in FUNCTION_POINTER_ABI_PATHS and FUNCTION_POINTER_TYPEDEF_RE.search(content):
        errors.append(f"{rel}: Power of 10 rule 9 forbids function pointer typedefs outside explicit ABI boundaries")
    return errors
