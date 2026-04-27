#!/usr/bin/env python3
# File: python_tools/core/models.py
# Purpose: Python quality and automation support for BLITZAR governance.

from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Protocol

JsonValue = None | bool | int | float | str | list["JsonValue"] | dict[str, "JsonValue"]
OptionsMap = dict[str, Any]


@dataclass(frozen=True)
# Description: Defines the CheckContext contract.
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
# Description: Defines the CheckResult contract.
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

    # Description: Executes the add_error operation.
    def add_error(self, message: str) -> None:
        self.ok = False
        self.errors.append(message)

    # Description: Executes the add_warning operation.
    def add_warning(self, message: str) -> None:
        self.warnings.append(message)


# Description: Defines the CheckContract contract.
class CheckContract(Protocol):
    # Description: Executes the run operation.
    def run(self, context: CheckContext) -> CheckResult:
        ...


# Description: Defines the CheckExecutionError contract.
class CheckExecutionError(RuntimeError):
    pass


# Description: Defines the ConfigurationError contract.
class ConfigurationError(CheckExecutionError):
    pass
