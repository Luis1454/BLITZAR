#!/usr/bin/env python3
# File: python_tools/policies/header_definition_policy.py
# Purpose: Python quality and automation support for BLITZAR governance.

from __future__ import annotations

import re

CONTROL_STATEMENT_RE = re.compile(r"\b(if|for|while|switch|catch)\s*\(")
TYPE_DECLARATION_RE = re.compile(r"^(?:class|struct|enum|union)\b")


def find_header_function_definition_lines(content: str) -> list[int]:
    lines = content.splitlines()
    matches: list[int] = []
    index = 0
    while index < len(lines):
        stripped = lines[index].strip()
        if not stripped or stripped.startswith(("#", "//", "/*", "*")) or "(" not in stripped:
            index += 1
            continue
        if TYPE_DECLARATION_RE.match(stripped):
            index += 1
            continue
        combined = stripped
        cursor = index
        while cursor + 1 < len(lines) and "{" not in combined and ";" not in combined:
            cursor += 1
            next_line = lines[cursor].strip()
            if not next_line or next_line.startswith(("//", "/*", "*")):
                continue
            combined = f"{combined} {next_line}"
        open_brace = combined.find("{")
        close_paren = combined.rfind(")")
        if open_brace != -1 and close_paren != -1 and open_brace > close_paren:
            if ";" not in combined[:open_brace] and not CONTROL_STATEMENT_RE.search(combined):
                matches.append(index + 1)
        index = cursor + 1 if cursor > index else index + 1
    return matches
