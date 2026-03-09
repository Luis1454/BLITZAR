#!/usr/bin/env python3
from __future__ import annotations

import shutil
import sys
import tempfile

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
        pytest_temp_dir = tempfile.mkdtemp(prefix="gravity-python-quality-")
        commands = (
            [sys.executable, "-m", "ruff", "check", "."],
            [sys.executable, "-m", "mypy", "tests/checks", "python_tools"],
            [sys.executable, "-m", "pytest", "--basetemp", pytest_temp_dir, "-q", "tests/checks/suites"],
        )
        try:
            for command in commands:
                completed = self._runner.run(command, cwd=context.root)
                if completed.returncode != 0:
                    result.add_error(f"command failed: {' '.join(command)}\n{completed.stdout}{completed.stderr}".strip())
        finally:
            shutil.rmtree(pytest_temp_dir, ignore_errors=True)


