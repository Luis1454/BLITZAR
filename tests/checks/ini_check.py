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
from python_tools.policies.ini_check import IniCheck


class IniCheckCommand(BaseCliCommand):
    def build_parser(self) -> argparse.ArgumentParser:
        parser = argparse.ArgumentParser(description="Validate simulation.ini values.")
        parser.add_argument("--config", required=True, help="Path to simulation.ini")
        return parser

    def build_context(self, args: argparse.Namespace) -> CheckContext:
        config = Path(args.config)
        if not config.is_absolute():
            config = (ROOT / config).resolve()
        return CheckContext(root=ROOT, config=config)

    def build_check(self, args: argparse.Namespace) -> BaseCheck:
        del args
        return IniCheck()


def main() -> int:
    return IniCheckCommand().main()


if __name__ == "__main__":
    raise SystemExit(main())

