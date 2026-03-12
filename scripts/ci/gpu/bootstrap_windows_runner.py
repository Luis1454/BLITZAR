#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path

from python_tools.ci.gpu_runner_bootstrap import WindowsGpuRunnerBootstrap


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Validate and bootstrap a Windows self-hosted GPU runner.")
    parser.add_argument("--repo", required=True, help="owner/repo")
    parser.add_argument("--runner-root", required=True, help="Path to extracted actions-runner directory")
    parser.add_argument("--runner-name", required=True, help="Runner name")
    parser.add_argument("--token-env", default="GPU_RUNNER_REG_TOKEN", help="Environment variable holding registration token")
    parser.add_argument("--label", action="append", default=["self-hosted", "windows", "x64", "cuda"], help="Runner label (repeatable)")
    parser.add_argument("--output", required=True, help="Output JSON plan path")
    parser.add_argument("--emit-script", default="", help="Optional PowerShell bootstrap script output path")
    parser.add_argument("--execute", action="store_true", help="Execute config.cmd and service commands after validation")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    bootstrap = WindowsGpuRunnerBootstrap()
    plan = bootstrap.plan(
        repo=args.repo,
        runner_root=Path(args.runner_root),
        runner_name=args.runner_name,
        token_env=args.token_env,
        labels=args.label,
    )
    bootstrap.write(plan, Path(args.output))
    if args.emit_script:
        bootstrap.emit_script(plan, Path(args.emit_script))
    if args.execute:
        bootstrap.execute(plan)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
