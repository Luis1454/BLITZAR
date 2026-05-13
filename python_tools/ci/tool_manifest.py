#!/usr/bin/env python3
# @file python_tools/ci/tool_manifest.py
# @author Luis1454
# @project BLITZAR
# @brief Python quality and automation support for BLITZAR governance.

from __future__ import annotations

import json
import platform
import subprocess
import sys
from collections.abc import Callable, Mapping
from pathlib import Path

from python_tools.ci.release_support import default_ci_context

CommandRunner = Callable[[list[str]], str]


# @brief Documents the default command runner operation contract.
# @param command Input value used by this contract.
# @return Value produced by this contract when applicable.
# @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
def default_command_runner(command: list[str]) -> str:
    try:
        completed = subprocess.run(command, capture_output=True, check=False, text=True)
    except FileNotFoundError as exc:
        raise RuntimeError(str(exc)) from exc
    output = completed.stdout.strip() or completed.stderr.strip()
    if completed.returncode != 0:
        raise RuntimeError(output or f"command failed: {' '.join(command)}")
    return output


# @brief Defines the tool manifest collector type contract.
# @param None This contract does not take explicit parameters.
# @note Keep construction and side effects explicit for deterministic quality gates.
class ToolManifestCollector:
    # @brief Documents the init operation contract.
    # @param command_runner Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def __init__(self, command_runner: CommandRunner | None = None) -> None:
        self._run = command_runner or default_command_runner

    # @brief Documents the collect operation contract.
    # @param lane Input value used by this contract.
    # @param profile Input value used by this contract.
    # @param compiler_command Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
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

    # @brief Documents the write operation contract.
    # @param manifest Input value used by this contract.
    # @param output_path Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def write(self, manifest: Mapping[str, object], output_path: Path) -> Path:
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_text(json.dumps(manifest, indent=2), encoding="utf-8")
        return output_path

    # @brief Documents the runner info operation contract.
    # @param None This contract does not take explicit parameters.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _runner_info(self) -> dict[str, str]:
        return {
            "os": platform.system(),
            "platform": platform.platform(),
            "release": platform.release(),
            "machine": platform.machine(),
        }

    # @brief Documents the tool entry operation contract.
    # @param command Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _tool_entry(self, command: list[str]) -> dict[str, object]:
        try:
            version = self._run(command)
            return {"status": "available", "command": command, "version": version}
        except RuntimeError as exc:
            return {"status": "unavailable", "command": command, "error": str(exc)}

    @staticmethod
    # @brief Documents the default compiler command operation contract.
    # @param None This contract does not take explicit parameters.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _default_compiler_command() -> list[str]:
        if platform.system().lower().startswith("win"):
            return ["cl"]
        return ["c++", "--version"]
