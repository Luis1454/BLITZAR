#!/usr/bin/env python3
# @file python_tools/core/runner.py
# @author Luis1454
# @project BLITZAR
# @brief Python quality and automation support for BLITZAR governance.

from __future__ import annotations

from collections.abc import Callable

from .base_check import BaseCheck
from .models import CheckContext
from .reporting import ResultReporter

CheckFactory = Callable[[], BaseCheck]


# @brief Documents the resolve sequence operation contract.
# @param check_name Input value used by this contract.
# @param with_launcher Input value used by this contract.
# @param sequences Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
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


# @brief Defines the check runner type contract.
# @param None This contract does not take explicit parameters.
# @note Keep construction and side effects explicit for deterministic quality gates.
class CheckRunner:
    # @brief Documents the init operation contract.
    # @param registry Input value used by this contract.
    # @param sequences Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def __init__(self, registry: dict[str, CheckFactory], sequences: dict[str, list[str]] | None = None) -> None:
        self._registry = registry
        self._sequences = sequences

    # @brief Documents the run operation contract.
    # @param check_name Input value used by this contract.
    # @param context Input value used by this contract.
    # @param reporter Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
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
