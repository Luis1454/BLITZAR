#!/usr/bin/env python3
from __future__ import annotations

from collections.abc import Callable

from .base_check import BaseCheck
from .models import CheckContext
from .reporting import ResultReporter

CheckFactory = Callable[[], BaseCheck]


def resolve_sequence(check_name: str, with_launcher: bool) -> list[str]:
    if check_name == "all":
        ordered = ["ini", "mirror", "no_legacy", "quality", "test_catalog", "pr_policy", "repo"]
        if with_launcher:
            ordered.insert(3, "launcher")
        return ordered
    if check_name == "quality":
        return ["quality", "test_catalog", "pr_policy"]
    return [check_name]


class CheckRunner:
    def __init__(self, registry: dict[str, CheckFactory]) -> None:
        self._registry = registry

    def run(self, check_name: str, context: CheckContext, reporter: ResultReporter | None = None) -> bool:
        current_reporter = reporter if reporter is not None else ResultReporter()
        all_ok = True
        for name in resolve_sequence(check_name, context.with_launcher):
            factory = self._registry.get(name)
            if factory is None:
                all_ok = False
                continue
            check = factory()
            all_ok &= current_reporter.emit(check.run(context))
        return all_ok
