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
from python_tools.policies.quality_baseline import QualityBaselineCheck


class QualityCheckCommand(BaseCliCommand):
    def build_parser(self) -> argparse.ArgumentParser:
        parser = argparse.ArgumentParser(description="Validate quality assurance baseline artifacts.")
        parser.add_argument("--root", required=True, help="Repository root")
        return parser

    def build_context(self, args: argparse.Namespace) -> CheckContext:
        return CheckContext(root=Path(args.root).resolve())

    def build_check(self, args: argparse.Namespace) -> BaseCheck:
        del args
        return QualityBaselineCheck()


def main() -> int:
    return QualityCheckCommand().main()


if __name__ == "__main__":
    raise SystemExit(main())

