#!/usr/bin/env python3
# @file python_tools/core/base_check.py
# @author Luis1454
# @project BLITZAR
# @brief Python quality and automation support for BLITZAR governance.

from __future__ import annotations

from abc import ABC, abstractmethod

from .models import CheckContext, CheckResult


# @brief Defines the base check type contract.
# @param None This contract does not take explicit parameters.
# @note Keep construction and side effects explicit for deterministic quality gates.
class BaseCheck(ABC):
    name = "check"
    success_message = ""
    failure_title = "Check failed:"
    warning_title = "Warnings:"

    # @brief Documents the new result operation contract.
    # @param None This contract does not take explicit parameters.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _new_result(self) -> CheckResult:
        return CheckResult(
            name=self.name,
            success_message=self.success_message,
            failure_title=self.failure_title,
            warning_title=self.warning_title,
        )

    # @brief Documents the run operation contract.
    # @param context Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def run(self, context: CheckContext) -> CheckResult:
        result = self._new_result()
        self._preflight(context, result)
        if not result.ok:
            return result
        self._execute(context, result)
        self._postprocess(context, result)
        return result

    # @brief Documents the preflight operation contract.
    # @param context Input value used by this contract.
    # @param result Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _preflight(self, context: CheckContext, result: CheckResult) -> None:
        del context, result

    @abstractmethod
    # @brief Documents the execute operation contract.
    # @param context Input value used by this contract.
    # @param result Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _execute(self, context: CheckContext, result: CheckResult) -> None:
        raise NotImplementedError

    # @brief Documents the postprocess operation contract.
    # @param context Input value used by this contract.
    # @param result Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _postprocess(self, context: CheckContext, result: CheckResult) -> None:
        del context, result

