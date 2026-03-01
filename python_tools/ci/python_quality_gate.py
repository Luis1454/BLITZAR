#!/usr/bin/env python3
from __future__ import annotations

import sys

from python_tools.core.base_check import BaseCheck
from python_tools.core.io import ProcessRunner
from python_tools.core.models import CheckContext, CheckResult


class PythonQualityGateCheck(BaseCheck):
    name = "python_quality"
    success_message = "python quality gate passed"
    failure_title = "python quality gate failed:"

    def __init__(self) -> None:
        self._runner = ProcessRunner()

    def _execute(self, context: CheckContext, result: CheckResult) -> None:
        commands = (
            [sys.executable, "-m", "pytest", "-q", "tests/checks/tests"],
            [sys.executable, "-m", "ruff", "check", "."],
            [sys.executable, "-m", "mypy", "tests/checks", "scripts/ci", "python_tools"],
        )
        for command in commands:
            completed = self._runner.run(command, cwd=context.root)
            if completed.returncode != 0:
                result.add_error(f"command failed: {' '.join(command)}\n{completed.stdout}{completed.stderr}".strip())

