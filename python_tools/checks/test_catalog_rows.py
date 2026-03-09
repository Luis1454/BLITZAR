#!/usr/bin/env python3
from __future__ import annotations

from python_tools.checks.quality_manifest import QUALITY_MANIFEST_PATH, QualityManifestLoader
from python_tools.checks.test_inventory import TestInventory
from python_tools.core.models import CheckResult
from python_tools.core.typing_ext import JsonValue


def manifest_rows(
    loader: QualityManifestLoader,
    manifest: dict[str, JsonValue],
    key: str,
    result: CheckResult,
    keyed_field: str,
) -> list[dict[str, JsonValue]]:
    return TestInventory(loader).manifest_rows(manifest, key, result, keyed_field)


def build_catalog_rows(
    loader: QualityManifestLoader,
    manifest: dict[str, JsonValue],
    result: CheckResult,
    groups_key: str,
) -> list[dict[str, JsonValue]]:
    return TestInventory(loader).build_catalog_rows(manifest, result, groups_key)


def _flatten_grouped_map(groups: dict[str, JsonValue], result: CheckResult) -> list[dict[str, JsonValue]]:
    rows: list[dict[str, JsonValue]] = []
    for source_ref, items in groups.items():
        if not isinstance(source_ref, str):
            result.add_error(f"{QUALITY_MANIFEST_PATH}: grouped test source keys must be strings")
            continue
        if not isinstance(items, dict):
            result.add_error(f"{QUALITY_MANIFEST_PATH}: grouped test source '{source_ref}' must map to object")
            continue
        rows.extend(_rows_from_group_dict(source_ref.strip(), items, result))
    return rows


def _rows_from_group_dict(source_ref: str, items: dict[str, JsonValue], result: CheckResult) -> list[dict[str, JsonValue]]:
    rows: list[dict[str, JsonValue]] = []
    for default_code, item in items.items():
        if not isinstance(default_code, str) or not default_code.strip():
            result.add_error(f"{QUALITY_MANIFEST_PATH}: grouped test code keys must be non-empty strings")
            continue
        if not isinstance(item, dict):
            result.add_error(f"{QUALITY_MANIFEST_PATH}: grouped test item '{default_code}' must be an object")
            continue
        rows.append(_row_from_item(item, source_ref, default_code.strip()))
    return rows


def _row_from_item(item: dict[str, JsonValue], source_ref: str, default_code: str) -> dict[str, JsonValue]:
    row = dict(item)
    if not isinstance(row.get("test_code"), str) or not str(row.get("test_code", "")).strip():
        row["test_code"] = default_code
    if "test_id" not in row:
        row["test_id"] = row.get("id", "")
    row["source"] = source_ref
    return row

