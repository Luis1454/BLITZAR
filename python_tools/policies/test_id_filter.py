#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path

from python_tools.core.io import collect_filtered_test_ids
from python_tools.core.models import CheckResult
from python_tools.core.typing_ext import JsonValue
from python_tools.policies.quality_manifest import QUALITY_MANIFEST_PATH


def collect_repo_quality_test_ids(
    root: Path,
    manifest: dict[str, JsonValue],
    result: CheckResult,
) -> set[str]:
    regex_pattern, files = _read_policy(manifest, result)
    if not regex_pattern or not files:
        return set()
    try:
        compiled = re.compile(regex_pattern)
    except re.error as exc:
        result.add_error(f"{QUALITY_MANIFEST_PATH}: invalid policies.test_ids.repo_quality.regex '{regex_pattern}': {exc}")
        return set()
    return collect_filtered_test_ids(root, compiled, tuple(files))


def _read_policy(manifest: dict[str, JsonValue], result: CheckResult) -> tuple[str, list[str]]:
    policies = manifest.get("policies")
    if not isinstance(policies, dict):
        return "", []
    test_ids = policies.get("test_ids")
    if not isinstance(test_ids, dict):
        return "", []
    repo_quality = test_ids.get("repo_quality")
    if not isinstance(repo_quality, dict):
        return "", []

    regex_pattern = repo_quality.get("regex")
    files = repo_quality.get("files")
    if not isinstance(regex_pattern, str) or not regex_pattern.strip():
        result.add_error(f"{QUALITY_MANIFEST_PATH}: policies.test_ids.repo_quality.regex must be a non-empty string")
        return "", []
    if not isinstance(files, list):
        result.add_error(f"{QUALITY_MANIFEST_PATH}: policies.test_ids.repo_quality.files must be a list")
        return "", []

    normalized_files: list[str] = []
    for item in files:
        if not isinstance(item, str) or not item.strip():
            result.add_error(f"{QUALITY_MANIFEST_PATH}: policies.test_ids.repo_quality.files entries must be non-empty strings")
            continue
        normalized_files.append(item.strip())
    return regex_pattern.strip(), normalized_files
