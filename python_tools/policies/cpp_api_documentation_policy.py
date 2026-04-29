#!/usr/bin/env python3
# @file python_tools/policies/cpp_api_documentation_policy.py
# @author Luis1454
# @project BLITZAR
# @brief Python quality and automation support for BLITZAR governance.

from __future__ import annotations

import re

from python_tools.core.base_check import BaseCheck
from python_tools.core.models import CheckContext, CheckResult

COMMENT_LINE_RE = re.compile(r"^\s*(?://+|/\*+|\*)\s*")
CONTROL_STATEMENT_RE = re.compile(r"\b(if|for|while|switch|catch)\s*\(")
TYPE_DECLARATION_RE = re.compile(r"^\s*(?:class|struct)\b")
CPP_EXTS = {".c", ".cc", ".cpp", ".cxx", ".cu", ".cuh", ".h", ".hh", ".hpp", ".hxx", ".inl"}
PUBLIC_HEADER_ROOTS = ("engine/include/", "runtime/include/", "modules/qt/include/ui/")
STYLE_ROOTS = ("apps/", "engine/", "runtime/", "modules/", "tests/")


# @brief Documents the is comment line operation contract.
# @param stripped Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def _is_comment_line(stripped: str) -> bool:
    return bool(COMMENT_LINE_RE.match(stripped))


# @brief Documents the leading comment block operation contract.
# @param lines Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def _leading_comment_block(lines: list[str]) -> str:
    index = 0
    while index < len(lines) and not lines[index].strip():
        index += 1

    block: list[str] = []
    while index < len(lines):
        stripped = lines[index].strip()
        if not stripped or not _is_comment_line(stripped):
            break
        block.append(stripped)
        index += 1
    return "\n".join(block)


# @brief Documents the comment block before operation contract.
# @param lines Input value used by this contract.
# @param line_index Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def _comment_block_before(lines: list[str], line_index: int) -> str:
    cursor = line_index - 1
    while cursor >= 0 and not lines[cursor].strip():
        cursor -= 1
    if cursor < 0 or not _is_comment_line(lines[cursor].strip()):
        return ""

    block: list[str] = []
    while cursor >= 0 and _is_comment_line(lines[cursor].strip()):
        block.append(lines[cursor].strip())
        cursor -= 1
    return "\n".join(reversed(block))


# @brief Documents the has file header operation contract.
# @param content Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def _has_file_header(content: str) -> bool:
    block = _leading_comment_block(content.splitlines())
    return ("@file" in block or "Module:" in block) and ("@brief" in block or "Responsibility:" in block)


# @brief Documents the is class definition operation contract.
# @param lines Input value used by this contract.
# @param line_index Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def _is_class_definition(lines: list[str], line_index: int) -> bool:
    stripped = lines[line_index].strip()
    if not TYPE_DECLARATION_RE.match(stripped) or stripped.endswith(";"):
        return False
    if "{" in stripped:
        return True

    cursor = line_index + 1
    while cursor < len(lines):
        next_stripped = lines[cursor].strip()
        if not next_stripped:
            cursor += 1
            continue
        if _is_comment_line(next_stripped):
            cursor += 1
            continue
        return "{" in next_stripped
    return False


# @brief Documents the is public signature operation contract.
# @param stripped Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def _looks_like_public_signature(stripped: str) -> bool:
    if "(" not in stripped or ")" not in stripped:
        return False
    if CONTROL_STATEMENT_RE.search(stripped):
        return False
    if stripped.startswith(("template ", "using ", "typedef ", "friend ")):
        return False
    return stripped.endswith((";", "{")) or "= default;" in stripped or "= delete;" in stripped


# @brief Documents the public header documentation scan operation contract.
# @param rel Input value used by this contract.
# @param content Input value used by this contract.
# @param result Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def _scan_public_header_documentation(rel: str, content: str, result: CheckResult) -> None:
    lines = content.splitlines()
    if not _has_file_header(content):
        result.add_error(
            f"{rel}: public C++ files must start with a responsibility block containing @file/@brief or Module/Responsibility"
        )

    brace_depth = 0
    class_depth = -1
    access = "private"

    for index, raw in enumerate(lines):
        stripped = raw.strip()
        if not stripped:
            brace_depth += raw.count("{") - raw.count("}")
            continue

        if _is_class_definition(lines, index):
            block = _comment_block_before(lines, index)
            if "@brief" not in block:
                result.add_error(f"{rel}:{index + 1}: class definition must be preceded by a @brief documentation block")
            if "{" in stripped:
                class_depth = brace_depth + raw.count("{") - raw.count("}")
                access = "private" if stripped.startswith("class ") else "public"

        elif class_depth >= 0 and brace_depth >= class_depth:
            if stripped == "public:":
                access = "public"
            elif stripped == "protected:":
                access = "protected"
            elif stripped == "private:":
                access = "private"
            elif access == "public" and _looks_like_public_signature(stripped):
                block = _comment_block_before(lines, index)
                if "@brief" not in block:
                    result.add_error(
                        f"{rel}:{index + 1}: public member function must be preceded by a @brief documentation block"
                    )

        elif brace_depth <= 1 and _looks_like_public_signature(stripped):
            block = _comment_block_before(lines, index)
            if "@brief" not in block:
                result.add_error(f"{rel}:{index + 1}: public function must be preceded by a @brief documentation block")

        brace_depth += raw.count("{") - raw.count("}")
        if class_depth >= 0 and brace_depth < class_depth:
            class_depth = -1
            access = "private"


# @brief Defines the cpp api documentation check type contract.
# @param None This contract does not take explicit parameters.
# @note Keep construction and side effects explicit for deterministic quality gates.
class CppApiDocumentationCheck(BaseCheck):
    name = "cpp_api_docs"
    success_message = "C++ API documentation check passed"
    failure_title = "C++ API documentation check failed:"

    # @brief Documents the execute operation contract.
    # @param context Input value used by this contract.
    # @param result Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _execute(self, context: CheckContext, result: CheckResult) -> None:
        for path in context.root.rglob("*"):
            if path.is_dir():
                continue
            rel = path.relative_to(context.root).as_posix()
            if not rel.startswith(STYLE_ROOTS):
                continue
            if path.suffix.lower() not in CPP_EXTS:
                continue

            content = path.read_text(encoding="utf-8", errors="ignore")
            if not _has_file_header(content):
                result.add_error(
                    f"{rel}: public C++ files must start with a responsibility block containing @file/@brief or Module/Responsibility"
                )
            if rel.startswith(PUBLIC_HEADER_ROOTS) and path.suffix.lower() in {".h", ".hh", ".hpp", ".hxx"}:
                _scan_public_header_documentation(rel, content, result)
