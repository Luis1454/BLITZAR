#!/usr/bin/env python3
from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Protocol

JsonValue = None | bool | int | float | str | list["JsonValue"] | dict[str, "JsonValue"]
OptionsMap = dict[str, Any]


@dataclass(frozen=True)
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
    paths: tuple[str, ...] = ()
    options: OptionsMap = field(default_factory=dict)


@dataclass
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

    def add_error(self, message: str) -> None:
        self.ok = False
        self.errors.append(message)

    def add_warning(self, message: str) -> None:
        self.warnings.append(message)


class CheckContract(Protocol):
    def run(self, context: CheckContext) -> CheckResult:
        ...


class CheckExecutionError(RuntimeError):
    pass


class ConfigurationError(CheckExecutionError):
    pass
