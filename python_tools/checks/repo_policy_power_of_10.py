#!/usr/bin/env python3
from __future__ import annotations

import re

GOTO_RE = re.compile(r"\bgoto\b")
SETJMP_LONGJMP_RE = re.compile(r"\b(?:setjmp|longjmp)\b")
DO_WHILE_RE = re.compile(r"\bdo\b\s*\{", re.DOTALL)
WHILE_TRUE_RE = re.compile(r"\bwhile\s*\(\s*true\s*\)")
OBJECT_LIKE_DEFINE_RE = re.compile(r"(?m)^\s*#define\s+([A-Z][A-Z0-9_]+)\b(?!\s*\()")
HEADER_GUARD_RE = re.compile(r"(?m)^\s*#ifndef\s+([A-Z][A-Z0-9_]+)\s*$")
FUNCTION_POINTER_TYPEDEF_RE = re.compile(r"(?m)^\s*(?:typedef|using)\b[^\n;]*\(\s*\*\s*[A-Za-z0-9_]*\s*\)")

ALLOWED_POWER_OF_10_MACROS = {"GRAVITY_HD", "GRAVITY_FRONTEND_MODULE_EXPORT", "NOMINMAX"}
FUNCTION_POINTER_ABI_PATHS = {"runtime/include/frontend/FrontendModuleApi.hpp"}


def check_power_of_10_content(rel: str, content: str) -> list[str]:
    errors: list[str] = []
    if GOTO_RE.search(content):
        errors.append(f"{rel}: Power of 10 rule 1 forbids goto in production paths")
    if SETJMP_LONGJMP_RE.search(content):
        errors.append(f"{rel}: Power of 10 rule 1 forbids setjmp/longjmp in production paths")
    if DO_WHILE_RE.search(content):
        errors.append(f"{rel}: Power of 10 rule 1 forbids do-while in production paths")
    if WHILE_TRUE_RE.search(content):
        errors.append(f"{rel}: Power of 10 rule 2 forbids open-ended while(true) loops in production paths")
    errors.extend(check_power_of_10_macros(rel, content))
    if rel not in FUNCTION_POINTER_ABI_PATHS and FUNCTION_POINTER_TYPEDEF_RE.search(content):
        errors.append(f"{rel}: Power of 10 rule 9 forbids function pointer typedefs outside explicit ABI boundaries")
    return errors


def check_power_of_10_macros(rel: str, content: str) -> list[str]:
    errors: list[str] = []
    header_guards = set(HEADER_GUARD_RE.findall(content))
    for macro_name in OBJECT_LIKE_DEFINE_RE.findall(content):
        if macro_name in header_guards or macro_name in ALLOWED_POWER_OF_10_MACROS:
            continue
        errors.append(
            f"{rel}: Power of 10 rule 8 forbids non-structural object-like macros in production paths ({macro_name})"
        )
    return errors

