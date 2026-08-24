#!/usr/bin/env python3
"""Run one executable under GDB and preserve the child exit status."""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path


def build_command(executable: Path, arguments: list[str]) -> list[str]:
    return [
        "gdb",
        "--quiet",
        "--batch",
        "--return-child-result",
        "-ex",
        "set pagination off",
        "-ex",
        "set confirm off",
        "-ex",
        "run",
        "-ex",
        "python exit_code = gdb.parse_and_eval('$_exitcode'); gdb.execute('bt full') if int(exit_code) != 0 else None",
        "--args",
        str(executable),
        *arguments,
    ]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("arguments", nargs=argparse.REMAINDER)
    parsed = parser.parse_args()
    arguments = parsed.arguments
    if arguments and arguments[0] == "--":
        arguments = arguments[1:]

    if shutil.which("gdb") is None:
        print("gdb_runner: gdb is not available", file=sys.stderr)
        return 2
    if not parsed.executable.is_file():
        print(f"gdb_runner: executable is missing: {parsed.executable}", file=sys.stderr)
        return 2

    result = subprocess.run(build_command(parsed.executable, arguments), check=False)
    return result.returncode


if __name__ == "__main__":
    sys.exit(main())
