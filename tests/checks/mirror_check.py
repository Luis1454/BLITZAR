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
from python_tools.policies.mirror_check import MirrorCheck


class MirrorCheckCommand(BaseCliCommand):
    def build_parser(self) -> argparse.ArgumentParser:
        parser = argparse.ArgumentParser(description="Validate header/source mirror layout.")
        parser.add_argument("--root", required=True, help="Repository root directory")
        return parser

    def build_context(self, args: argparse.Namespace) -> CheckContext:
        return CheckContext(root=Path(args.root).resolve())

    def build_check(self, args: argparse.Namespace) -> BaseCheck:
        del args
        return MirrorCheck()


def main() -> int:
    return MirrorCheckCommand().main()


if __name__ == "__main__":
    raise SystemExit(main())

