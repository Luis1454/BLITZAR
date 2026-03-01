#!/usr/bin/env python3
from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from python_tools.core.base_check import BaseCheck
from python_tools.core.base_command import BaseCliCommand
from python_tools.core.models import CheckContext
from python_tools.policies.no_legacy_check import NoLegacyCheck


class NoLegacyCommand(BaseCliCommand):
    def build_parser(self) -> argparse.ArgumentParser:
        parser = argparse.ArgumentParser(description="Validate no-legacy frontend policy.")
        parser.add_argument("--root", required=True, help="Repository root directory")
        parser.add_argument("--build-dir", default="", help="Build directory containing build.ninja")
        parser.add_argument("--check-build-targets", action="store_true", help="Also validate generated targets in build.ninja")
        return parser

    def build_context(self, args: argparse.Namespace) -> CheckContext:
        build_dir = Path(args.build_dir).resolve() if args.build_dir else None
        return CheckContext(root=Path(args.root).resolve(), build_dir=build_dir, check_build_targets=args.check_build_targets)

    def build_check(self, args: argparse.Namespace) -> BaseCheck:
        del args
        return NoLegacyCheck()


def main() -> int:
    return NoLegacyCommand().main()


if __name__ == "__main__":
    raise SystemExit(main())

