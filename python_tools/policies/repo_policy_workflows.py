#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path

from python_tools.core.models import CheckResult

EVIDENCE_WORKFLOW_PATHS = (
    ".github/workflows/pr-fast-quality-gate.yml",
    ".github/workflows/nightly-full.yml",
    ".github/workflows/release-lane.yml",
)
WORKFLOW_ROOT = ".github/workflows/"
CI_PYTHON_REQUIREMENTS = ".github/ci/requirements-py312.txt"
LEGACY_CTEST_SELECTORS = ("ConfigArgsTest", "ServerProtocolTest", "ClientBridgeTest", "ClientRuntimeTest", "QtMainWindowTest")
WORKFLOW_USES_RE = re.compile(r"(?m)^\s*(?:-\s*)?uses:\s*([^\s#]+)")
ACTION_SHA_RE = re.compile(r"^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+(?:/[A-Za-z0-9_.-]+)*@[0-9a-f]{40}$")


def check_evidence_workflow_commands(
    root: Path,
    result: CheckResult,
    marker_text: str,
    required_text: str,
    error_text: str,
) -> None:
    for rel in EVIDENCE_WORKFLOW_PATHS:
        path = root / rel
        if not path.exists():
            continue
        content = path.read_text(encoding="utf-8", errors="ignore")
        for command in collect_marker_commands(content, marker_text):
            if required_text in command:
                continue
            result.add_error(f"{rel}: {error_text}: {command}")


def check_legacy_ctest_selectors(root: Path, result: CheckResult) -> None:
    for rel in EVIDENCE_WORKFLOW_PATHS:
        path = root / rel
        if not path.exists():
            continue
        content = path.read_text(encoding="utf-8", errors="ignore")
        for command in collect_marker_commands(content, "ctest "):
            if any(selector in command for selector in LEGACY_CTEST_SELECTORS):
                result.add_error(f"{rel}: CI ctest selector must use normalized TST_* ids: {command}")


def check_workflow_action_pinning(root: Path, result: CheckResult) -> None:
    workflows_root = root / WORKFLOW_ROOT
    if not workflows_root.exists():
        return
    for path in sorted(workflows_root.glob("*.yml")):
        rel = path.relative_to(root).as_posix()
        content = path.read_text(encoding="utf-8", errors="ignore")
        for match in WORKFLOW_USES_RE.finditer(content):
            target = match.group(1).strip()
            if target.startswith("./"):
                continue
            if not ACTION_SHA_RE.match(target):
                result.add_error(f"{rel}: workflow actions must pin full commit SHAs: {target}")


def check_workflow_pip_manifest_usage(root: Path, result: CheckResult) -> None:
    workflows_root = root / WORKFLOW_ROOT
    if not workflows_root.exists():
        return
    for path in sorted(workflows_root.glob("*.yml")):
        rel = path.relative_to(root).as_posix()
        content = path.read_text(encoding="utf-8", errors="ignore")
        for command in collect_marker_commands(content, "pip install"):
            if CI_PYTHON_REQUIREMENTS not in command:
                result.add_error(f"{rel}: workflow pip installs must use {CI_PYTHON_REQUIREMENTS}: {command}")


def check_workflow_failure_masking(root: Path, result: CheckResult) -> None:
    workflows_root = root / WORKFLOW_ROOT
    if not workflows_root.exists():
        return
    for path in sorted(workflows_root.glob("*.yml")):
        rel = path.relative_to(root).as_posix()
        content = path.read_text(encoding="utf-8", errors="ignore")
        for marker in ("cmake --build", "ctest "):
            for command in collect_marker_commands(content, marker):
                if "||" in command:
                    result.add_error(
                        f"{rel}: workflow must not mask build/test command failures with shell fallbacks: {command}"
                    )


def check_release_lane_subset(root: Path, result: CheckResult) -> None:
    path = root / ".github/workflows/release-lane.yml"
    if not path.exists():
        return
    content = path.read_text(encoding="utf-8", errors="ignore")
    for marker in ("TST_UNT_PROT_", "TST_UNT_MODCLI_", "TST_UNT_PHYS_"):
        if marker not in content:
            result.add_error(
                ".github/workflows/release-lane.yml: release lane must exercise an explicit deterministic "
                f"product subset containing {marker}"
            )


def collect_marker_commands(content: str, marker_text: str) -> list[str]:
    commands: list[str] = []
    lines = content.splitlines()
    index = 0
    while index < len(lines):
        stripped = lines[index].strip()
        marker = stripped.find(marker_text)
        if marker == -1:
            index += 1
            continue
        command = stripped[marker:]
        while command.endswith("\\") and index + 1 < len(lines):
            index += 1
            command = f"{command} {lines[index].strip()}"
        commands.append(command)
        index += 1
    return commands
