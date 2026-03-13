#!/usr/bin/env python3
from __future__ import annotations

import os
from dataclasses import dataclass
from pathlib import Path

from python_tools.core.base_check import BaseCheck
from python_tools.core.io import ProcessRunner
from python_tools.core.models import CheckContext, CheckResult


@dataclass(frozen=True)
class LauncherCase:
    name: str
    expected_code: int
    args: list[str]
    must_stdout: str = ""
    must_stderr: str = ""


class LauncherContractCheck(BaseCheck):
    name = "launcher"
    success_message = "Launcher contract check passed."
    failure_title = "Launcher contract check failed:"

    def __init__(self) -> None:
        self._runner = ProcessRunner()

    def _execute(self, context: CheckContext, result: CheckResult) -> None:
        build_dir = context.build_dir
        if build_dir is None:
            result.add_error("missing build directory")
            return
        suffix = ".exe" if os.name == "nt" else ""
        launcher = build_dir.resolve() / f"aster{suffix}"
        if not launcher.exists():
            result.add_error(f"Launcher not found: {launcher}")
            return
        for case in self._cases():
            self._run_case(launcher, case, result)

    def _cases(self) -> tuple[LauncherCase, ...]:
        return (
            LauncherCase("launcher-help", 0, ["--help"], must_stdout="--mode client|server|headless"),
            LauncherCase("launcher-invalid-mode", 2, ["--mode", "nope"], must_stderr="invalid --mode value"),
            LauncherCase("launcher-headless-help", 0, ["--mode", "headless", "--", "--help"], must_stdout="--target-steps"),
            LauncherCase("launcher-headless-positional-rejected", 2, ["--mode", "headless", "--", "1000"], must_stderr="unexpected positional argument"),
            LauncherCase("launcher-server-unknown-option-rejected", 2, ["--mode", "server", "--", "--foo", "1"], must_stderr="unknown option: --foo"),
            LauncherCase("launcher-server-non-loopback-rejected", 2, ["--mode", "server", "--", "--server-host", "0.0.0.0"], must_stderr="refusing non-loopback bind host"),
        )

    def _run_case(self, launcher: Path, case: LauncherCase, result: CheckResult) -> None:
        completed = self._runner.run([str(launcher), *case.args], timeout=20)
        if completed.returncode != case.expected_code:
            result.add_error(
                f"[{case.name}] unexpected exit code: got {completed.returncode}, expected {case.expected_code}\n"
                f"stdout:\n{completed.stdout}\n"
                f"stderr:\n{completed.stderr}"
            )
        if case.must_stdout and case.must_stdout not in completed.stdout:
            result.add_error(f"[{case.name}] missing expected stdout fragment: {case.must_stdout}\nstdout:\n{completed.stdout}")
        if case.must_stderr and case.must_stderr not in completed.stderr:
            result.add_error(f"[{case.name}] missing expected stderr fragment: {case.must_stderr}\nstderr:\n{completed.stderr}")

