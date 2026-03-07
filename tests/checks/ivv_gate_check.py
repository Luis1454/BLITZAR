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
from python_tools.policies.ivv_gate import IvvGateCheck


class IvvGateCommand(BaseCliCommand):
    def build_parser(self) -> argparse.ArgumentParser:
        parser = argparse.ArgumentParser(description="Validate IV&V independence gate for PRs.")
        parser.add_argument("--event-name", default=os.getenv("GITHUB_EVENT_NAME", ""), help="GitHub event name")
        parser.add_argument("--event-path", default=os.getenv("GITHUB_EVENT_PATH", ""), help="Path to GitHub event payload JSON")
        parser.add_argument("--repo", default=os.getenv("GITHUB_REPOSITORY", ""), help="GitHub repository owner/name")
        parser.add_argument("--token", default=os.getenv("GITHUB_TOKEN", ""), help="GitHub token used to query PR metadata")
        return parser

    def build_context(self, args: argparse.Namespace) -> CheckContext:
        return CheckContext(
            root=ROOT,
            event_name=args.event_name,
            event_path=args.event_path,
            options={"repo": args.repo, "token": args.token},
        )

    def build_check(self, args: argparse.Namespace) -> BaseCheck:
        del args
        return IvvGateCheck()


def main() -> int:
    return IvvGateCommand().main()


if __name__ == "__main__":
    raise SystemExit(main())
