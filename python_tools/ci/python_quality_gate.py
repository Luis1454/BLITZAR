#!/usr/bin/env python3
# File: python_tools/ci/python_quality_gate.py
# Purpose: Python quality and automation support for BLITZAR governance.

from __future__ import annotations

import shutil
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
        pytest_temp_dir = context.root / ".pytest-basetemp-python-quality"
        pytest_temp = str(pytest_temp_dir)
        shutil.rmtree(pytest_temp_dir, ignore_errors=True)
        commands = (
            [sys.executable, "-m", "ruff", "check", "tests/checks", "scripts/ci", "python_tools"],
            [sys.executable, "-m", "mypy", "tests/checks", "scripts/ci", "python_tools"],
            [sys.executable, "-m", "pytest", "--basetemp", pytest_temp, "-q", "tests/checks/suites"],
        )
        for command in commands:
            completed = self._runner.run(command, cwd=context.root)
            if completed.returncode != 0:
                result.add_error(f"command failed: {' '.join(command)}\n{completed.stdout}{completed.stderr}".strip())
        shutil.rmtree(pytest_temp_dir, ignore_errors=True)

