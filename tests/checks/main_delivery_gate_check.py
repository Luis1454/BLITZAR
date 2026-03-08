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
from python_tools.policies.main_delivery_policy import MainDeliveryGateCheck


class MainDeliveryGateCommand(BaseCliCommand):
    def build_parser(self) -> argparse.ArgumentParser:
        parser = argparse.ArgumentParser(description="Validate that pushes to main originate from merged issue pull requests.")
        parser.add_argument("--event-name", default=os.getenv("GITHUB_EVENT_NAME", ""), help="GitHub event name")
        parser.add_argument("--event-path", default=os.getenv("GITHUB_EVENT_PATH", ""), help="Path to GitHub event payload JSON")
        parser.add_argument("--repo", default=os.getenv("GITHUB_REPOSITORY", ""), help="owner/repo")
        parser.add_argument("--sha", default=os.getenv("GITHUB_SHA", ""), help="Commit SHA to validate")
        return parser

    def build_context(self, args: argparse.Namespace) -> CheckContext:
        return CheckContext(
            root=ROOT,
            event_name=args.event_name,
            event_path=args.event_path,
            options={"repo": args.repo, "token": os.getenv("GITHUB_TOKEN", ""), "sha": args.sha},
        )

    def build_check(self, args: argparse.Namespace) -> BaseCheck:
        del args
        return MainDeliveryGateCheck()


def main() -> int:
    return MainDeliveryGateCommand().main()


if __name__ == "__main__":
    raise SystemExit(main())
