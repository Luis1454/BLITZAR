#!/usr/bin/env python3
from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from python_tools.ci.fmea_status import FmeaStatusSnapshot


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Package nightly FMEA risk status snapshot.")
    parser.add_argument("--root", default=".", help="Repository root")
    parser.add_argument("--dist-dir", default="dist/fmea-status", help="Output directory for FMEA status files")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    archive = FmeaStatusSnapshot().package(root=Path(args.root), dist_dir=Path(args.dist_dir))
    print(archive.as_posix())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
