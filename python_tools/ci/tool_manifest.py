#!/usr/bin/env python3
from __future__ import annotations

import json
import platform
import subprocess
import sys
from collections.abc import Callable, Mapping
from pathlib import Path

from python_tools.ci.release_evidence_defaults import default_ci_context

CommandRunner = Callable[[list[str]], str]


def default_command_runner(command: list[str]) -> str:
    try:
        completed = subprocess.run(command, capture_output=True, check=False, text=True)
    except FileNotFoundError as exc:
        raise RuntimeError(str(exc)) from exc
    output = completed.stdout.strip() or completed.stderr.strip()
    if completed.returncode != 0:
        raise RuntimeError(output or f"command failed: {' '.join(command)}")
    return output


class ToolManifestCollector:
    def __init__(self, command_runner: CommandRunner | None = None) -> None:
        self._run = command_runner or default_command_runner

    def collect(self, lane: str, profile: str, compiler_command: list[str] | None = None) -> dict[str, object]:
        compiler = compiler_command or self._default_compiler_command()
        return {
            "format_version": "1.0",
            "lane": lane,
            "profile": profile,
            "runner": self._runner_info(),
            "tools": {
                "python": self._tool_entry([sys.executable, "--version"]),
                "cmake": self._tool_entry(["cmake", "--version"]),
                "clang_tidy": self._tool_entry(["clang-tidy", "--version"]),
                "compiler": self._tool_entry(compiler),
            },
            "ci_context": default_ci_context(),
        }

    def write(self, manifest: Mapping[str, object], output_path: Path) -> Path:
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_text(json.dumps(manifest, indent=2), encoding="utf-8")
        return output_path

    def _runner_info(self) -> dict[str, str]:
        return {
            "os": platform.system(),
            "platform": platform.platform(),
            "release": platform.release(),
            "machine": platform.machine(),
        }

    def _tool_entry(self, command: list[str]) -> dict[str, object]:
        try:
            version = self._run(command)
            return {"status": "available", "command": command, "version": version}
        except RuntimeError as exc:
            return {"status": "unavailable", "command": command, "error": str(exc)}

    @staticmethod
    def _default_compiler_command() -> list[str]:
        if platform.system().lower().startswith("win"):
            return ["cl"]
        return ["c++", "--version"]
