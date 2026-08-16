#!/usr/bin/env python3
"""CLI for a read-only inventory of generated local BLITZAR outputs."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from python_tools.ci.local_artifacts import build_report


def main() -> int:
    parser = argparse.ArgumentParser(description="Inventory generated local artifacts without deleting them.")
    parser.add_argument("--root", type=Path, default=Path.cwd(), help="repository root")
    parser.add_argument("--output", type=Path, help="optional JSON report path")
    args = parser.parse_args()
    report = json.dumps(build_report(args.root.resolve()), indent=2) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(report, encoding="utf-8")
    else:
        print(report, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
