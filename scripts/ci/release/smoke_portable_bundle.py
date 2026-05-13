#!/usr/bin/env python3
# @file scripts/ci/release/smoke_portable_bundle.py
# @author Luis1454
# @project BLITZAR
# @brief Build, release, and CI helper automation for BLITZAR workflows.

from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from python_tools.ci.release_bundle import ReleaseBundleSmokeValidator


# @brief Documents the parse args operation contract.
# @param None This contract does not take explicit parameters.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Validate and smoke-run a packaged portable Windows bundle.")
    parser.add_argument("--archive", required=True, help="Portable bundle archive produced by package_bundle.py")
    parser.add_argument(
        "--extract-dir",
        default="",
        help="Optional extraction directory. Defaults to a sibling directory next to the archive.",
    )
    parser.add_argument(
        "--layout-only",
        action="store_true",
        help="Validate extracted bundle contents without executing the packaged binaries.",
    )
    return parser.parse_args()


# @brief Documents the main operation contract.
# @param None This contract does not take explicit parameters.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def main() -> int:
    args = parse_args()
    archive = Path(args.archive)
    extract_dir = Path(args.extract_dir) if args.extract_dir.strip() else None
    validator = ReleaseBundleSmokeValidator()
    bundle_root = validator.validate_archive(archive, extract_dir=extract_dir, run_commands=not args.layout_only)
    print(bundle_root.as_posix())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
