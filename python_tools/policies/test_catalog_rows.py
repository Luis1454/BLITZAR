#!/usr/bin/env python3
from __future__ import annotations

from python_tools.core.models import CheckResult
from python_tools.core.typing_ext import JsonValue
from python_tools.policies.quality_manifest import QUALITY_MANIFEST_PATH, QualityManifestLoader


def manifest_rows(
    loader: QualityManifestLoader,
    manifest: dict[str, JsonValue],
    key: str,
    result: CheckResult,
    keyed_field: str,
) -> list[dict[str, JsonValue]]:
    return loader.get_list(manifest, key, result, keyed_field=keyed_field)


def build_catalog_rows(
    loader: QualityManifestLoader,
    manifest: dict[str, JsonValue],
    result: CheckResult,
    groups_key: str,
) -> list[dict[str, JsonValue]]:
    groups = manifest.get(groups_key)
    if not isinstance(groups, dict):
        result.add_error(f"{QUALITY_MANIFEST_PATH}: '{groups_key}' must be an object")
        return []
    return _flatten_grouped_map(groups, result)


def _flatten_grouped_map(groups: dict[str, JsonValue], result: CheckResult) -> list[dict[str, JsonValue]]:
    rows: list[dict[str, JsonValue]] = []
    for source, group_value in groups.items():
        source_ref = source.strip() if isinstance(source, str) else ""
        if isinstance(group_value, dict):
            rows.extend(_rows_from_group_dict(source_ref, group_value, result))
            continue
        result.add_error(f"{QUALITY_MANIFEST_PATH}: grouped test source '{source_ref or '<missing>'}' must map to object")
    return rows


def _rows_from_group_dict(source_ref: str, items: dict[str, JsonValue], result: CheckResult) -> list[dict[str, JsonValue]]:
    rows: list[dict[str, JsonValue]] = []
    for test_code, item in items.items():
        if not isinstance(item, dict):
            result.add_error(
                f"{QUALITY_MANIFEST_PATH}: grouped test item '{test_code}' for source '{source_ref or '<missing>'}' must be an object"
            )
            continue
        rows.append(_row_from_item(item, source_ref, test_code.strip()))
    return rows


def _row_from_item(item: dict[str, JsonValue], source_ref: str, default_code: str) -> dict[str, JsonValue]:
    return {
        "test_code": item.get("test_code", default_code),
        "test_id": item.get("id", ""),
        "req_ids": item.get("req_ids", []),
        "source": source_ref,
    }
