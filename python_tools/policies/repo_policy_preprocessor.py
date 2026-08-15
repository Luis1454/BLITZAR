#!/usr/bin/env python3
# @file python_tools/policies/repo_policy_preprocessor.py
# @author Luis1454
# @project BLITZAR
# @brief Bounded preprocessor validation for repository policy checks.

from __future__ import annotations

import re

PREPROCESSOR_CONDITIONAL_RE = re.compile(r"(?m)^\s*#(?:if|ifdef|ifndef|elif|else|endif)\b")
PREPROCESSOR_OPEN_RE = re.compile(r"^\s*#(?:if|ifdef|ifndef|elif)\b")
ALLOWED_HARDWARE_DIRECTIVE_RE = re.compile(
    r"^\s*#(?:if\s+(?:BLITZAR_(?:ENABLE_CUDA|HAS_CUDA_DRIVER|HAS_NVRTC)|defined\(__CUDA_ARCH__\))"
    r"|ifdef\s+(?:__CUDACC__|__SSE__)"
    r"|ifndef\s+BLITZAR_HD_(?:HOST|DEVICE)"
    r"|define\s+BLITZAR_HD_(?:HOST|DEVICE)(?:\s+.*)?"
    r"|else|endif)"
)


# @brief Reports whether a preprocessor directive is limited to a qualified hardware seam.
# @param line Directive source line.
# @return True when the directive is an approved CUDA or ISA feature boundary.
def is_allowed_hardware_directive(line: str) -> bool:
    return bool(ALLOWED_HARDWARE_DIRECTIVE_RE.match(line))


# @brief Reports whether conditional compilation introduces a non-hardware branch.
# @param content C++ source content.
# @return True when a conditional opening directive is outside the hardware seam contract.
def has_unapproved_preprocessor_conditional(content: str) -> bool:
    for line in content.splitlines():
        if PREPROCESSOR_OPEN_RE.match(line) and not is_allowed_hardware_directive(line):
            return True
    return False
