#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

from python_tools.checks.test_inventory import TestInventory
from python_tools.core.models import CheckResult
from python_tools.core.typing_ext import JsonValue


def collect_repo_quality_test_ids(
    root: Path,
    manifest: dict[str, JsonValue],
    result: CheckResult,
) -> set[str]:
    return TestInventory().collect_repo_quality_test_ids(root, manifest, result)


def _read_policy(manifest: dict[str, JsonValue], result: CheckResult) -> tuple[str, list[str]]:
    return TestInventory().read_repo_quality_policy(manifest, result)

