#!/usr/bin/env python3
from __future__ import annotations

import json
import sys
from pathlib import Path

from python_tools.ci.tool_manifest import ToolManifestCollector


def _fake_runner(outputs: dict[tuple[str, ...], str]):
    def run(command: list[str]) -> str:
        key = tuple(command)
        if key not in outputs:
            raise RuntimeError(f"missing mock output for {' '.join(command)}")
        return outputs[key]

    return run


def test_tool_manifest_collector_records_versions(tmp_path: Path) -> None:
    collector = ToolManifestCollector(
        _fake_runner(
                {
                (sys.executable, "--version"): "Python 3.12.0",
                ("cmake", "--version"): "cmake version 3.30.1",
                ("clang-tidy", "--version"): "LLVM clang-tidy 18.1.8",
                ("c++", "--version"): "g++ 14.2.0",
            }
        )
    )
    manifest = collector.collect(lane="pr-fast", profile="prod", compiler_command=["c++", "--version"])

    assert manifest["lane"] == "pr-fast"
    tools = manifest["tools"]
    assert isinstance(tools, dict)
    assert tools["python"]["status"] == "available"
    assert tools["compiler"]["version"] == "g++ 14.2.0"

    output = collector.write(manifest, tmp_path / "tool_manifest.json")
    saved = json.loads(output.read_text(encoding="utf-8"))
    assert saved["profile"] == "prod"


def test_tool_manifest_collector_marks_missing_tool_unavailable() -> None:
    collector = ToolManifestCollector(_fake_runner({(sys.executable, "--version"): "Python 3.12.0"}))
    manifest = collector.collect(lane="release", profile="prod", compiler_command=["c++", "--version"])

    tools = manifest["tools"]
    assert isinstance(tools, dict)
    assert tools["cmake"]["status"] == "unavailable"
    assert tools["clang_tidy"]["status"] == "unavailable"
