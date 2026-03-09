#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path

from python_tools.checks.quality_manifest import QUALITY_MANIFEST_PATH, REQUIREMENTS_KEY, QualityManifestLoader
from python_tools.core.io import REQ_ID_RE, TEST_MACRO_RE, collect_filtered_test_ids, collect_test_ids
from python_tools.core.models import CheckResult
from python_tools.core.typing_ext import JsonValue


class TestInventory:
    def __init__(self, loader: QualityManifestLoader | None = None) -> None:
        self._loader = loader or QualityManifestLoader()

    def collect_known_test_ids(
        self,
        root: Path,
        manifest: dict[str, JsonValue],
        result: CheckResult,
        override: object = None,
    ) -> set[str]:
        extra_ids = override if isinstance(override, set) else self.collect_repo_quality_test_ids(root, manifest, result)
        return collect_test_ids(root, extra_ids, TEST_MACRO_RE)

    def collect_repo_quality_test_ids(
        self,
        root: Path,
        manifest: dict[str, JsonValue],
        result: CheckResult,
    ) -> set[str]:
        regex_pattern, files = self.read_repo_quality_policy(manifest, result)
        if not regex_pattern or not files:
            return set()
        try:
            compiled = re.compile(regex_pattern)
        except re.error as exc:
            result.add_error(f"{QUALITY_MANIFEST_PATH}: invalid policies.test_ids.repo_quality.regex '{regex_pattern}': {exc}")
            return set()
        return collect_filtered_test_ids(root, compiled, tuple(files))

    def read_repo_quality_policy(
        self,
        manifest: dict[str, JsonValue],
        result: CheckResult,
    ) -> tuple[str, list[str]]:
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

    def manifest_rows(
        self,
        manifest: dict[str, JsonValue],
        key: str,
        result: CheckResult,
        keyed_field: str,
    ) -> list[dict[str, JsonValue]]:
        return self._loader.get_list(manifest, key, result, keyed_field=keyed_field)

    def requirement_ids(self, manifest: dict[str, JsonValue], result: CheckResult) -> set[str]:
        rows = self.manifest_rows(manifest, REQUIREMENTS_KEY, result, keyed_field="id")
        req_ids: set[str] = set()
        for row in rows:
            req_id = row.get("id")
            if isinstance(req_id, str) and REQ_ID_RE.match(req_id.strip()):
                req_ids.add(req_id.strip())
        return req_ids

    def build_catalog_rows(
        self,
        manifest: dict[str, JsonValue],
        result: CheckResult,
        groups_key: str,
    ) -> list[dict[str, JsonValue]]:
        groups = manifest.get(groups_key)
        if not isinstance(groups, dict):
            result.add_error(f"{QUALITY_MANIFEST_PATH}: '{groups_key}' must be an object")
            return []
        rows: list[dict[str, JsonValue]] = []
        for source, group_value in groups.items():
            source_ref = source.strip() if isinstance(source, str) else ""
            if not isinstance(group_value, dict):
                result.add_error(f"{QUALITY_MANIFEST_PATH}: grouped test source '{source_ref or '<missing>'}' must map to object")
                continue
            for test_code, item in group_value.items():
                if not isinstance(item, dict):
                    result.add_error(
                        f"{QUALITY_MANIFEST_PATH}: grouped test item '{test_code}' for source '{source_ref or '<missing>'}' must be an object"
                    )
                    continue
                rows.append(
                    {
                        "test_code": item.get("test_code", test_code.strip()),
                        "test_id": item.get("id", ""),
                        "req_ids": item.get("req_ids", []),
                        "source": source_ref,
                    }
                )
        return rows
