#!/usr/bin/env python3
# File: python_tools/core/runner.py
# Purpose: Python quality and automation support for BLITZAR governance.

from __future__ import annotations

from collections.abc import Callable

from .base_check import BaseCheck
from .models import CheckContext
from .reporting import ResultReporter

CheckFactory = Callable[[], BaseCheck]


# Description: Executes the resolve_sequence operation.
def resolve_sequence(check_name: str, with_launcher: bool, sequences: dict[str, list[str]] | None = None) -> list[str]:
    if sequences is not None and check_name in sequences:
        ordered = list(sequences[check_name])
        if check_name == "all" and with_launcher and "launcher" not in ordered:
            ordered.insert(3, "launcher")
        return ordered
    if check_name == "all":
        ordered = ["ini", "mirror", "no_legacy", "quality", "test_catalog", "pr_policy", "repo"]
        if with_launcher:
            ordered.insert(3, "launcher")
        return ordered
    if check_name == "quality":
        return ["quality", "test_catalog", "pr_policy"]
    return [check_name]


# Description: Defines the CheckRunner contract.
class CheckRunner:
    # Description: Executes the __init__ operation.
    def __init__(self, registry: dict[str, CheckFactory], sequences: dict[str, list[str]] | None = None) -> None:
        self._registry = registry
        self._sequences = sequences

    # Description: Executes the run operation.
    def run(self, check_name: str, context: CheckContext, reporter: ResultReporter | None = None) -> bool:
        current_reporter = reporter if reporter is not None else ResultReporter()
        all_ok = True
        for name in resolve_sequence(check_name, context.with_launcher, self._sequences):
            factory = self._registry.get(name)
            if factory is None:
                all_ok = False
                continue
            check = factory()
            all_ok &= current_reporter.emit(check.run(context))
        return all_ok
