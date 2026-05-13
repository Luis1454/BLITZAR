#!/usr/bin/env python3
# @file python_tools/core/reporting.py
# @author Luis1454
# @project BLITZAR
# @brief Python quality and automation support for BLITZAR governance.

from __future__ import annotations

import sys

from .models import CheckResult


# @brief Defines the result reporter type contract.
# @param None This contract does not take explicit parameters.
# @note Keep construction and side effects explicit for deterministic quality gates.
class ResultReporter:
    # @brief Documents the emit operation contract.
    # @param result Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def emit(self, result: CheckResult) -> bool:
        if result.stdout:
            print(result.stdout, end="")
        if result.stderr:
            print(result.stderr, end="", file=sys.stderr)
        if result.warnings:
            print(result.warning_title)
            for warning in result.warnings:
                print(f"  - {warning}")
        if result.errors:
            print(result.failure_title, file=sys.stderr)
            for error in result.errors:
                print(f"  - {error}", file=sys.stderr)
            return False
        if result.success_message:
            print(result.success_message)
        return result.ok


# @brief Documents the emit result operation contract.
# @param result Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def emit_result(result: CheckResult) -> bool:
    return ResultReporter().emit(result)

