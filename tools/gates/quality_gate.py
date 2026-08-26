"""Run the manifest-defined deterministic repository gates."""

from __future__ import annotations

import argparse
import json
import pathlib
import shlex
import subprocess
import sys


def load_checks(root: pathlib.Path) -> list[dict[str, object]]:
    quality_path = root / "plan" / "quality.json"
    quality = json.loads(quality_path.read_text(encoding="utf-8"))
    checks = quality.get("checks")
    if not isinstance(checks, list):
        raise ValueError("quality manifest checks must be a list")
    return [check for check in checks if isinstance(check, dict)]


def select_checks(
    checks: list[dict[str, object]], group: str
) -> list[dict[str, object]]:
    selected = [
        check for check in checks if str(check.get("group", "static")) == group
    ]
    if not selected:
        raise ValueError(f"quality gate group is empty: {group}")
    return selected


def command_arguments(command: str) -> list[str]:
    arguments = shlex.split(command)
    if arguments and arguments[0] in {"python", "python3"}:
        arguments[0] = sys.executable
    return arguments


def run_checks(root: pathlib.Path, checks: list[dict[str, object]]) -> int:
    failures = 0
    for check in checks:
        check_id = str(check.get("id", "unknown"))
        command = check.get("command")
        if not isinstance(command, str) or not command:
            print(f"quality-gate: {check_id} has no command", file=sys.stderr)
            failures += 1
            continue
        print(f"quality-gate: running {check_id}: {command}", flush=True)
        result = subprocess.run(command_arguments(command), cwd=root, check=False)
        if result.returncode != 0:
            print(
                f"quality-gate: {check_id} failed with {result.returncode}",
                file=sys.stderr,
            )
            failures += 1
    return failures


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=pathlib.Path, required=True)
    parser.add_argument("--group", default="static")
    arguments = parser.parse_args(argv)
    root = arguments.root.resolve()
    try:
        checks = select_checks(load_checks(root), arguments.group)
        failures = run_checks(root, checks)
    except (OSError, ValueError, json.JSONDecodeError) as error:
        print(f"quality-gate: {error}", file=sys.stderr)
        return 1
    print(
        f"quality-gate: group={arguments.group}, checks={len(checks)}, "
        f"failures={failures}"
    )
    return int(bool(failures))


if __name__ == "__main__":
    sys.exit(main())
