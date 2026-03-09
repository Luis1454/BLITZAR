#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

from python_tools.core.base_check import BaseCheck
from python_tools.core.models import CheckContext, CheckResult
from python_tools.policies.header_definition_policy import find_header_function_definition_lines
from python_tools.policies.repo_policy_support import (
    CPP_SCAN_EXTS,
    EVIDENCE_WORKFLOW_PATHS,
    FORBIDDEN_CPP_EXTS,
    FORBIDDEN_MARKER_RE,
    HEADER_EXTS,
    LEGACY_CTEST_SELECTORS,
    LINE_COUNT_NAMES,
    TEXT_EXTS,
    check_cpp_content,
    collect_marker_commands,
    load_allowlist,
    should_count_lines,
    should_skip_dir,
)


class RepoPolicyCheck(BaseCheck):
    name = "repo"
    success_message = "Repository policy check passed"
    failure_title = "Repository policy check failed:"
    warning_title = "Repository policy warnings:"

    def _execute(self, context: CheckContext, result: CheckResult) -> None:
        allowlist = load_allowlist(context.allowlist if context.allowlist is not None else context.root / "tests/checks/policy_allowlist.txt")
        used_allowlist: set[str] = set()
        for path in context.root.rglob("*"):
            if path.is_dir():
                continue
            rel_parts = path.relative_to(context.root).parts
            if any(should_skip_dir(part) for part in rel_parts):
                continue
            rel = path.relative_to(context.root).as_posix()
            suffix = path.suffix.lower()
            content: str | None = None
            if suffix in FORBIDDEN_CPP_EXTS:
                result.add_error(f"{rel}: forbidden C/C++ extension '{suffix}' (use .hpp/.cpp)")
            if suffix in TEXT_EXTS or path.name in LINE_COUNT_NAMES:
                content = path.read_text(encoding="utf-8", errors="ignore")
                if suffix in HEADER_EXTS:
                    for line_number in find_header_function_definition_lines(content):
                        result.add_error(f"{rel}:{line_number}: function definitions in headers are forbidden")
                marker_match = FORBIDDEN_MARKER_RE.search(content)
                if marker_match:
                    result.add_error(f"{rel}: forbidden marker '{marker_match.group(2)}' found")
                if suffix in CPP_SCAN_EXTS:
                    check_cpp_content(rel, content, result)
            if not should_count_lines(path):
                continue
            if content is None:
                content = path.read_text(encoding="utf-8", errors="ignore")
            self._check_line_count(rel, content, context, allowlist, used_allowlist, result)
        for stale in sorted(allowlist - used_allowlist):
            result.add_warning(f"allowlist entry not needed anymore: {stale}")
        self._check_evidence_workflow_commands(context.root, result, "cmake -S", "-DGRAVITY_PROFILE=prod", "evidence configure command must include -DGRAVITY_PROFILE=prod")
        self._check_evidence_workflow_commands(context.root, result, "ctest ", "--no-tests=error", "CI ctest command must include --no-tests=error")
        self._check_legacy_ctest_selectors(context.root, result)

    def _check_line_count(self, rel: str, content: str, context: CheckContext, allowlist: set[str], used_allowlist: set[str], result: CheckResult) -> None:
        line_count = len(content.splitlines())
        if line_count > context.hard_lines:
            if rel in allowlist:
                used_allowlist.add(rel)
            else:
                result.add_error(f"{rel}: {line_count} lines exceeds hard limit {context.hard_lines} (split file or document explicit exception)")
            return
        if line_count > context.target_lines:
            result.add_warning(f"{rel}: {line_count} lines exceeds target {context.target_lines}")

    def _check_evidence_workflow_commands(self, root: Path, result: CheckResult, marker_text: str, required_text: str, error_text: str) -> None:
        for rel in EVIDENCE_WORKFLOW_PATHS:
            path = root / rel
            if not path.exists():
                continue
            content = path.read_text(encoding="utf-8", errors="ignore")
            for command in collect_marker_commands(content, marker_text):
                if required_text not in command:
                    result.add_error(f"{rel}: {error_text}: {command}")

    def _check_legacy_ctest_selectors(self, root: Path, result: CheckResult) -> None:
        for rel in EVIDENCE_WORKFLOW_PATHS:
            path = root / rel
            if not path.exists():
                continue
            content = path.read_text(encoding="utf-8", errors="ignore")
            for command in collect_marker_commands(content, "ctest "):
                if any(selector in command for selector in LEGACY_CTEST_SELECTORS):
                    result.add_error(f"{rel}: CI ctest selector must use normalized TST_* ids: {command}")
