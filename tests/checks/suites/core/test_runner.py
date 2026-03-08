#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

import python_tools.core.runner as runner_module
from python_tools.core.base_check import BaseCheck
from python_tools.core.models import CheckContext, CheckResult
from python_tools.core.runner import CheckRunner, resolve_sequence


class _StaticCheck(BaseCheck):
    def __init__(self, ok: bool) -> None:
        self._ok = ok
        self.name = "static"

    def _execute(self, context: CheckContext, result: CheckResult) -> None:
        del context
        if not self._ok:
            result.add_error("failed")


def _context(tmp_path: Path) -> CheckContext:
    return CheckContext(root=tmp_path, with_launcher=False)


def test_resolve_sequence_all_without_launcher() -> None:
    assert resolve_sequence("all", False) == ["ini", "mirror", "no_legacy", "quality", "test_catalog", "pr_policy", "repo"]


def test_resolve_sequence_all_with_launcher() -> None:
    assert resolve_sequence("all", True) == ["ini", "mirror", "no_legacy", "launcher", "quality", "test_catalog", "pr_policy", "repo"]


def test_runner_dispatches_and_aggregates(monkeypatch, tmp_path: Path) -> None:
    calls: list[str] = []

    def _factory(name: str, ok: bool):
        def _build() -> BaseCheck:
            calls.append(name)
            return _StaticCheck(ok)

        return _build

    monkeypatch.setattr(runner_module, "ResultReporter", lambda: type("R", (), {"emit": staticmethod(lambda r: r.ok)})())
    runner = CheckRunner({"quality": _factory("quality", True), "test_catalog": _factory("test_catalog", False), "pr_policy": _factory("pr_policy", True)})
    assert runner.run("quality", _context(tmp_path)) is False
    assert calls == ["quality", "test_catalog", "pr_policy"]


def test_runner_uses_custom_catalog_sequences(monkeypatch, tmp_path: Path) -> None:
    calls: list[str] = []

    def _factory(name: str):
        def _build() -> BaseCheck:
            calls.append(name)
            return _StaticCheck(True)

        return _build

    monkeypatch.setattr(runner_module, "ResultReporter", lambda: type("R", (), {"emit": staticmethod(lambda r: r.ok)})())
    runner = CheckRunner({"alpha": _factory("alpha"), "beta": _factory("beta")}, sequences={"bundle": ["beta", "alpha"]})

    assert runner.run("bundle", _context(tmp_path)) is True
    assert calls == ["beta", "alpha"]

