#!/usr/bin/env python3
# File: python_tools/ci/gpu_runner_bootstrap.py
# Purpose: Python quality and automation support for BLITZAR governance.

from __future__ import annotations

import json
import os
import shutil
import subprocess
from pathlib import Path
from typing import Any, cast


# Description: Defines the WindowsGpuRunnerBootstrap contract.
class WindowsGpuRunnerBootstrap:
    REQUIRED_TOOLS = ("python", "cmake", "ninja", "nvidia-smi", "nvcc")
    REQUIRED_FILES = ("config.cmd", "run.cmd", "svc.cmd")

    # Description: Executes the plan operation.
    def plan(
        self,
        repo: str,
        runner_root: Path,
        runner_name: str,
        token_env: str,
        labels: list[str],
    ) -> dict[str, Any]:
        root = runner_root.resolve()
        missing_files = [name for name in self.REQUIRED_FILES if not (root / name).is_file()]
        if missing_files:
            raise FileNotFoundError(f"runner root missing required files: {', '.join(missing_files)}")
        missing_tools = [tool for tool in self.REQUIRED_TOOLS if shutil.which(tool) is None]
        if missing_tools:
            raise RuntimeError(f"missing required tools: {', '.join(missing_tools)}")
        config_command = [
            str(root / "config.cmd"),
            "--unattended",
            "--replace",
            "--url",
            f"https://github.com/{repo}",
            "--token",
            f"%{token_env}%",
            "--name",
            runner_name,
            "--labels",
            ",".join(labels),
        ]
        return {
            "repo": repo,
            "runner_root": str(root),
            "runner_name": runner_name,
            "token_env": token_env,
            "labels": labels,
            "required_tools": list(self.REQUIRED_TOOLS),
            "commands": {
                "configure": config_command,
                "install_service": [str(root / "svc.cmd"), "install"],
                "start_service": [str(root / "svc.cmd"), "start"],
            },
        }

    @staticmethod
    # Description: Executes the write operation.
    def write(plan: dict[str, Any], output_path: Path) -> Path:
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_text(json.dumps(plan, indent=2), encoding="utf-8")
        return output_path

    @staticmethod
    # Description: Executes the emit_script operation.
    def emit_script(plan: dict[str, Any], output_path: Path) -> Path:
        commands = cast(dict[str, list[str]], plan["commands"])
        script = [
            "$ErrorActionPreference = 'Stop'",
            f"if (-not $env:{plan['token_env']}) {{ throw 'Missing runner registration token in {plan['token_env']}' }}",
            "& " + WindowsGpuRunnerBootstrap._quote_command(commands["configure"]),
            "& " + WindowsGpuRunnerBootstrap._quote_command(commands["install_service"]),
            "& " + WindowsGpuRunnerBootstrap._quote_command(commands["start_service"]),
        ]
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_text("\n".join(script) + "\n", encoding="utf-8")
        return output_path

    @staticmethod
    # Description: Executes the execute operation.
    def execute(plan: dict[str, Any]) -> None:
        token_env = cast(str, plan["token_env"])
        commands = cast(dict[str, list[str]], plan["commands"])
        if not os.environ.get(token_env):
            raise RuntimeError(f"missing runner registration token in {token_env}")
        for command in (
            commands["configure"],
            commands["install_service"],
            commands["start_service"],
        ):
            completed = subprocess.run(command, check=False)
            if completed.returncode != 0:
                raise RuntimeError(f"bootstrap command failed: {' '.join(command)}")

    @staticmethod
    # Description: Executes the _quote_command operation.
    def _quote_command(command: list[str]) -> str:
        return " ".join(f'"{part}"' if " " in part else part for part in command)
