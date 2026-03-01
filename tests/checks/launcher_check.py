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
from python_tools.policies.launcher_check import LauncherContractCheck


class LauncherCheckCommand(BaseCliCommand):
    def build_parser(self) -> argparse.ArgumentParser:
        parser = argparse.ArgumentParser(description="Validate launcher contract.")
        parser.add_argument("--build-dir", required=True, help="Build directory containing myApp binary")
        return parser

    def build_context(self, args: argparse.Namespace) -> CheckContext:
        return CheckContext(root=ROOT, build_dir=Path(args.build_dir).resolve())

    def build_check(self, args: argparse.Namespace) -> BaseCheck:
        del args
        return LauncherContractCheck()


def main() -> int:
    return LauncherCheckCommand().main()


if __name__ == "__main__":
    raise SystemExit(main())

