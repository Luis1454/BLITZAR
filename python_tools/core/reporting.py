#!/usr/bin/env python3
from __future__ import annotations

import sys

from .models import CheckResult


class ResultReporter:
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


def emit_result(result: CheckResult) -> bool:
    return ResultReporter().emit(result)

