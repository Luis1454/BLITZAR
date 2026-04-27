#!/usr/bin/env python3
# File: python_tools/core/base_check.py
# Purpose: Python quality and automation support for BLITZAR governance.

from __future__ import annotations

from abc import ABC, abstractmethod

from .models import CheckContext, CheckResult


# Description: Defines the BaseCheck contract.
class BaseCheck(ABC):
    name = "check"
    success_message = ""
    failure_title = "Check failed:"
    warning_title = "Warnings:"

    # Description: Executes the _new_result operation.
    def _new_result(self) -> CheckResult:
        return CheckResult(
            name=self.name,
            success_message=self.success_message,
            failure_title=self.failure_title,
            warning_title=self.warning_title,
        )

    # Description: Executes the run operation.
    def run(self, context: CheckContext) -> CheckResult:
        result = self._new_result()
        self._preflight(context, result)
        if not result.ok:
            return result
        self._execute(context, result)
        self._postprocess(context, result)
        return result

    # Description: Executes the _preflight operation.
    def _preflight(self, context: CheckContext, result: CheckResult) -> None:
        del context, result

    @abstractmethod
    # Description: Executes the _execute operation.
    def _execute(self, context: CheckContext, result: CheckResult) -> None:
        raise NotImplementedError

    # Description: Executes the _postprocess operation.
    def _postprocess(self, context: CheckContext, result: CheckResult) -> None:
        del context, result

