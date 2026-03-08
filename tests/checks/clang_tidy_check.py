#!/usr/bin/env python3
from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from python_tools.ci.clang_tidy import ClangTidyCheck
from python_tools.core.base_check import BaseCheck
from python_tools.core.base_command import BaseCliCommand
from python_tools.core.models import CheckContext


class ClangTidyCommand(BaseCliCommand):
    def build_parser(self) -> argparse.ArgumentParser:
        parser = argparse.ArgumentParser(description="Run clang-tidy over selected compile database entries.")
        parser.add_argument("--build-dir", required=True, help="Build directory containing compile_commands.json")
        parser.add_argument("--root", default=".", help="Repository root")
        parser.add_argument("--clang-tidy", default="clang-tidy", help="clang-tidy executable")
        parser.add_argument(
            "--checks",
            default="-*,clang-analyzer-*,bugprone-unused-return-value",
            help="clang-tidy checks expression",
        )
        parser.add_argument("--path", dest="paths", action="append", default=[], help="Restrict analysis to this repo-relative path (repeatable)")
        return parser

    def build_context(self, args: argparse.Namespace) -> CheckContext:
        return CheckContext(
            root=Path(args.root).resolve(),
            build_dir=Path(args.build_dir).resolve(),
            clang_tidy_binary=args.clang_tidy,
            clang_tidy_checks=args.checks,
            paths=tuple(args.paths),
        )

    def build_check(self, args: argparse.Namespace) -> BaseCheck:
        del args
        return ClangTidyCheck()


def main() -> int:
    return ClangTidyCommand().main()


if __name__ == "__main__":
    raise SystemExit(main())
