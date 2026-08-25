#!/usr/bin/env python3
"""Run one executable under a native debugger and preserve its status."""

from __future__ import annotations

import argparse
import os
import platform
import re
import shutil
import subprocess
import sys
from pathlib import Path


BACKENDS = ("gdb", "cdb", "lldb")
LLDB_EXIT_STATUS_PATTERN = re.compile(r"Process \d+ exited with status = (-?\d+)")


def candidate_backends(system_name: str) -> tuple[str, ...]:
    if system_name == "Windows":
        return ("cdb", "gdb", "lldb")
    if system_name == "Darwin":
        return ("lldb", "gdb", "cdb")
    return ("gdb", "lldb", "cdb")


def select_backend(
    requested: str, system_name: str, available: dict[str, str]
) -> tuple[str, str]:
    if requested != "auto":
        if requested not in BACKENDS:
            raise ValueError(f"unsupported debugger backend: {requested}")
        if requested not in available:
            raise RuntimeError(f"debugger backend is unavailable: {requested}")
        return requested, available[requested]

    for backend in candidate_backends(system_name):
        if backend in available:
            return backend, available[backend]

    raise RuntimeError(
        f"no supported debugger is available for {system_name}: "
        f"{', '.join(candidate_backends(system_name))}"
    )


def resolve_backend(requested: str, system_name: str) -> tuple[str, str]:
    configured_backend = os.environ.get("BLITZAR_DEBUGGER", "")
    configured_path = os.environ.get("BLITZAR_DEBUGGER_PATH", "")
    effective_request = requested

    if effective_request == "auto" and configured_backend:
        effective_request = configured_backend

    names = (effective_request,) if effective_request != "auto" else candidate_backends(system_name)
    available: dict[str, str] = {}

    for name in names:
        if name == configured_backend and configured_path:
            available[name] = configured_path
            continue

        executable = shutil.which(name)
        if executable is not None:
            available[name] = executable

    return select_backend(effective_request, system_name, available)


def build_gdb_command(
    debugger: str, executable: Path, arguments: list[str]
) -> list[str]:
    return [
        debugger,
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
        "python inferior = gdb.selected_inferior(); gdb.execute('bt full') if inferior.pid > 0 else None",
        "--args",
        str(executable),
        *arguments,
    ]


def build_cdb_command(
    debugger: str, executable: Path, arguments: list[str]
) -> list[str]:
    return [
        debugger,
        "-g",
        "-G",
        "-c",
        "sxe av; g; k; q",
        str(executable),
        *arguments,
    ]


def build_lldb_command(
    debugger: str, executable: Path, arguments: list[str]
) -> list[str]:
    return [
        debugger,
        "--batch",
        "--no-lldbinit",
        "--one-line-on-crash",
        "bt all",
        "--one-line",
        "run",
        "--",
        str(executable),
        *arguments,
    ]


def build_command(
    executable: Path, arguments: list[str], backend: str, debugger: str
) -> list[str]:
    builders = {
        "gdb": build_gdb_command,
        "cdb": build_cdb_command,
        "lldb": build_lldb_command,
    }

    try:
        builder = builders[backend]
    except KeyError as error:
        raise ValueError(f"unsupported debugger backend: {backend}") from error

    return builder(debugger, executable, arguments)


def evaluate_exit_code(
    actual: int, expected: int | None, executable: Path
) -> int:
    if expected is None:
        return actual
    if actual == expected:
        return 0

    print(
        f"debug_runner: expected exit code {expected} from {executable}, got {actual}",
        file=sys.stderr,
    )
    return 1


def parse_lldb_exit_status(output: str) -> int | None:
    matches = LLDB_EXIT_STATUS_PATTERN.findall(output)
    if not matches:
        return None
    return int(matches[-1])


def run_debugger(command: list[str], backend: str, timeout: float) -> int:
    if backend != "lldb":
        return subprocess.run(command, check=False, timeout=timeout).returncode

    result = subprocess.run(
        command,
        check=False,
        timeout=timeout,
        capture_output=True,
        text=True,
    )
    sys.stdout.write(result.stdout)
    sys.stderr.write(result.stderr)

    target_status = parse_lldb_exit_status(result.stdout + result.stderr)
    if target_status is not None:
        return target_status
    return result.returncode


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--backend", choices=("auto", *BACKENDS), default="auto")
    parser.add_argument("--timeout", type=float, default=120.0)
    parser.add_argument("--expected-exit-code", type=int)
    parser.add_argument("--capability", action="store_true")
    parser.add_argument("executable", type=Path, nargs="?")
    parser.add_argument("arguments", nargs=argparse.REMAINDER)
    parsed = parser.parse_args()

    if parsed.timeout <= 0.0:
        parser.error("--timeout must be positive")

    if parsed.arguments and parsed.arguments[0] == "--":
        parsed.arguments = parsed.arguments[1:]

    if not parsed.capability and parsed.executable is None:
        parser.error("an executable is required unless --capability is used")

    return parsed


def main() -> int:
    parsed = parse_arguments()

    try:
        backend, debugger = resolve_backend(parsed.backend, platform.system())
    except (RuntimeError, ValueError) as error:
        print(f"debug_runner: {error}", file=sys.stderr)
        return 2

    if parsed.capability:
        print(f"backend={backend}")
        print(f"debugger={debugger}")
        return 0

    if not parsed.executable.is_file():
        print(f"debug_runner: executable is missing: {parsed.executable}", file=sys.stderr)
        return 2

    command = build_command(parsed.executable, parsed.arguments, backend, debugger)
    print(f"debug_runner: backend={backend} debugger={debugger}", file=sys.stderr)

    try:
        result_code = run_debugger(command, backend, parsed.timeout)
    except subprocess.TimeoutExpired:
        print(
            f"debug_runner: timeout after {parsed.timeout:g}s: {parsed.executable}",
            file=sys.stderr,
        )
        return 124
    except OSError as error:
        print(f"debug_runner: failed to start debugger: {error}", file=sys.stderr)
        return 2

    return evaluate_exit_code(
        result_code, parsed.expected_exit_code, parsed.executable
    )


if __name__ == "__main__":
    sys.exit(main())
