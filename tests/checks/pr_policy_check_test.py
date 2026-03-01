#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from python_tools.core.base_check import BaseCheck
from python_tools.core.base_command import BaseCliCommand
from python_tools.core.models import CheckContext
from python_tools.policies.pr_policy import PrPolicySelfTestCheck


class PrPolicySelfTestCommand(BaseCliCommand):
    def build_parser(self):
        from argparse import ArgumentParser

        return ArgumentParser(description="Run PR policy check self-test fixtures.")

    def build_context(self, args):
        del args
        return CheckContext(root=ROOT)

    def build_check(self, args) -> BaseCheck:
        del args
        return PrPolicySelfTestCheck()


def main() -> int:
    return PrPolicySelfTestCommand().main()


if __name__ == "__main__":
    raise SystemExit(main())

