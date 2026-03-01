#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from python_tools.core.base_check import BaseCheck
from python_tools.core.base_command import BaseCliCommand
from python_tools.core.models import CheckContext
from python_tools.policies.pr_policy import PrPolicyCheck


class PrPolicyCommand(BaseCliCommand):
    def build_parser(self) -> argparse.ArgumentParser:
        parser = argparse.ArgumentParser(description="Validate AGENTS PR policy (branch/title/body coherence).")
        parser.add_argument("--event-name", default=os.getenv("GITHUB_EVENT_NAME", ""), help="GitHub event name")
        parser.add_argument("--event-path", default=os.getenv("GITHUB_EVENT_PATH", ""), help="Path to GitHub event payload JSON")
        parser.add_argument("--branch", default="", help="Override PR branch name (issue/<N>-<slug>)")
        parser.add_argument("--title", default="", help="Override PR title")
        parser.add_argument("--body", default="", help="Override PR body")
        return parser

    def build_context(self, args: argparse.Namespace) -> CheckContext:
        return CheckContext(
            root=ROOT,
            event_name=args.event_name,
            event_path=args.event_path,
            branch=args.branch,
            title=args.title,
            body=args.body,
        )

    def build_check(self, args: argparse.Namespace) -> BaseCheck:
        del args
        return PrPolicyCheck()


def main() -> int:
    return PrPolicyCommand().main()


if __name__ == "__main__":
    raise SystemExit(main())

