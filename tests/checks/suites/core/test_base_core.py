#!/usr/bin/env python3
from __future__ import annotations

from argparse import ArgumentParser
from pathlib import Path

from python_tools.core.base_check import BaseCheck
from python_tools.core.base_command import BaseCliCommand
from python_tools.core.models import CheckContext, CheckResult
from python_tools.core.reporting import ResultReporter


class _FlowCheck(BaseCheck):
    name = "flow"

    def __init__(self) -> None:
        self.calls: list[str] = []

    def _preflight(self, context: CheckContext, result: CheckResult) -> None:
        del context, result
        self.calls.append("pre")

    def _execute(self, context: CheckContext, result: CheckResult) -> None:
        del context, result
        self.calls.append("run")

    def _postprocess(self, context: CheckContext, result: CheckResult) -> None:
        del context, result
        self.calls.append("post")


class _StaticCommand(BaseCliCommand):
    def build_parser(self) -> ArgumentParser:
        return ArgumentParser(description="test")

    def build_context(self, args) -> CheckContext:
        del args
        return CheckContext(root=Path(".").resolve())

    def build_check(self, args) -> BaseCheck:
        del args
        return _FlowCheck()


def test_base_check_template_method_order() -> None:
    check = _FlowCheck()
    result = check.run(CheckContext(root=Path(".").resolve()))
    assert result.ok
    assert check.calls == ["pre", "run", "post"]


def test_result_reporter_handles_success(capsys) -> None:
    ok = ResultReporter().emit(CheckResult(name="x", success_message="ok"))
    captured = capsys.readouterr()
    assert ok
    assert "ok" in captured.out


def test_base_cli_command_main_returns_zero() -> None:
    assert _StaticCommand().main([]) == 0

