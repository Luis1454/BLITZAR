#!/usr/bin/env python3
# @file python_tools/core/models.py
# @author Luis1454
# @project BLITZAR
# @brief Python quality and automation support for BLITZAR governance.

from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Protocol

JsonValue = None | bool | int | float | str | list["JsonValue"] | dict[str, "JsonValue"]
OptionsMap = dict[str, Any]


@dataclass(frozen=True)
# @brief Defines the check context type contract.
# @param None This contract does not take explicit parameters.
# @note Keep construction and side effects explicit for deterministic quality gates.
class CheckContext:
    root: Path
    config: Path | None = None
    build_dir: Path | None = None
    allowlist: Path | None = None
    check_build_targets: bool = False
    with_launcher: bool = False
    target_lines: int = 200
    hard_lines: int = 300
    event_name: str = ""
    event_path: str = ""
    branch: str = ""
    title: str = ""
    body: str = ""
    clang_tidy_binary: str = "clang-tidy"
    clang_tidy_checks: str = "-*,clang-analyzer-*,bugprone-unused-return-value"
    clang_tidy_jobs: int = 0
    clang_tidy_log_dir: Path | None = None
    clang_tidy_diff_base: str = ""
    clang_tidy_diff_target: str = ""
    clang_tidy_header_filter: str = ""
    clang_tidy_file_timeout_sec: int = 0
    clang_tidy_timeout_fallback_checks: str = ""
    paths: tuple[str, ...] = ()
    options: OptionsMap = field(default_factory=dict)


@dataclass
# @brief Defines the check result type contract.
# @param None This contract does not take explicit parameters.
# @note Keep construction and side effects explicit for deterministic quality gates.
class CheckResult:
    name: str
    ok: bool = True
    success_message: str = ""
    failure_title: str = "Check failed:"
    warning_title: str = "Warnings:"
    warnings: list[str] = field(default_factory=list)
    errors: list[str] = field(default_factory=list)
    stdout: str = ""
    stderr: str = ""

    # @brief Documents the add error operation contract.
    # @param message Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def add_error(self, message: str) -> None:
        self.ok = False
        self.errors.append(message)

    # @brief Documents the add warning operation contract.
    # @param message Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def add_warning(self, message: str) -> None:
        self.warnings.append(message)


# @brief Defines the check contract type contract.
# @param None This contract does not take explicit parameters.
# @note Keep construction and side effects explicit for deterministic quality gates.
class CheckContract(Protocol):
    # @brief Documents the run operation contract.
    # @param context Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def run(self, context: CheckContext) -> CheckResult:
        ...


# @brief Defines the check execution error type contract.
# @param None This contract does not take explicit parameters.
# @note Keep construction and side effects explicit for deterministic quality gates.
class CheckExecutionError(RuntimeError):
    pass


# @brief Defines the configuration error type contract.
# @param None This contract does not take explicit parameters.
# @note Keep construction and side effects explicit for deterministic quality gates.
class ConfigurationError(CheckExecutionError):
    pass
