#!/usr/bin/env python3
from __future__ import annotations

from abc import ABC, abstractmethod
from argparse import ArgumentParser, Namespace

from .base_check import BaseCheck
from .models import CheckContext
from .reporting import ResultReporter


class BaseCliCommand(ABC):
    @abstractmethod
    def build_parser(self) -> ArgumentParser:
        raise NotImplementedError

    @abstractmethod
    def build_context(self, args: Namespace) -> CheckContext:
        raise NotImplementedError

    @abstractmethod
    def build_check(self, args: Namespace) -> BaseCheck:
        raise NotImplementedError

    def build_reporter(self) -> ResultReporter:
        return ResultReporter()

    def main(self, argv: list[str] | None = None) -> int:
        parser = self.build_parser()
        args = parser.parse_args(argv)
        context = self.build_context(args)
        check = self.build_check(args)
        reporter = self.build_reporter()
        return 0 if reporter.emit(check.run(context)) else 1

