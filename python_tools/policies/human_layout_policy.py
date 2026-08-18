"""Static checks for readable local declarations and raw ownership mistakes."""

from __future__ import annotations

import re

MULTI_DECLARATION_RE = re.compile(
    r"(?:^|[{};])\s*(?:const\s+)?(?:auto|[A-Za-z_][A-Za-z0-9_:<>]*)\s+"
    r"[A-Za-z_][A-Za-z0-9_]*\s*=[^;(),{}\[\]]*"
    r"(?:,[^;(),{}\[\]]*)+;"
)
RAW_OWNER_RE = re.compile(
    r"(?:^|[{};])\s*(?:const\s+)?[A-Za-z_][A-Za-z0-9_:<>]*\s*\*\s*"
    r"[A-Za-z_][A-Za-z0-9_]*\s*=\s*new\b"
)
RAW_POINTER_DECLARATION_RE = re.compile(
    r"\b(?:const\s+)?(?:auto|[A-Za-z_][A-Za-z0-9_:<>]*)\s*\*\s*"
    r"(?:const\s+)?[A-Za-z_][A-Za-z0-9_]*\s*(?=[=;,)({])"
)

RAW_POINTER_BOUNDARIES = (
    "runtime/client/module/CliApi.hpp",
    "runtime/client/module/CliBoundary.hpp",
)

STRICT_RAW_POINTER_PATHS = (
    "modules/qt/window/config/",
    "modules/qt/window/scene/",
)


def is_pointer_boundary(relative: str) -> bool:
    return (
        relative in RAW_POINTER_BOUNDARIES
        or relative.startswith("modules/qt/")
        or "/module/" in relative
        or relative.startswith("apps/")
    )


def check_human_layout(relative: str, content: str) -> list[str]:
    if "/tests/" in f"/{relative}" or relative.startswith("tests/"):
        return []
    errors: list[str] = []
    for line_number, line in enumerate(content.splitlines(), 1):
        if MULTI_DECLARATION_RE.search(line):
            errors.append(f"{relative}:{line_number}: one local declaration per statement is required")
        if RAW_OWNER_RE.search(line) and not is_pointer_boundary(relative):
            errors.append(f"{relative}:{line_number}: owning raw pointer must use std::unique_ptr")
        if relative.startswith(STRICT_RAW_POINTER_PATHS) and RAW_POINTER_DECLARATION_RE.search(line):
            errors.append(f"{relative}:{line_number}: raw pointer declaration is forbidden in internal scene code")
    return errors
