#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

from python_tools.core.io import JsonLoader
from python_tools.core.models import CheckResult
from python_tools.core.typing_ext import JsonValue

QUALITY_MANIFEST_PATH = "docs/quality/quality_manifest.json"
REQUIREMENTS_KEY = "requirements"
TESTS_KEY = "tests"
CROSSWALK_KEY = "crosswalk"


class QualityManifestLoader:
    def __init__(self) -> None:
        self._json = JsonLoader()

    def load(self, root: Path, result: CheckResult) -> dict[str, JsonValue]:
        payload, errors = self.load_with_errors(root)
        for error in errors:
            result.add_error(error)
        return payload

    def load_with_errors(self, root: Path) -> tuple[dict[str, JsonValue], list[str]]:
        errors: list[str] = []
        manifest_path = (root / QUALITY_MANIFEST_PATH).resolve()
        payload = self._load_manifest_object(root.resolve(), manifest_path, (), errors)
        if payload is None:
            return {}, errors
        return payload, errors

    def _load_manifest_object(
        self,
        root: Path,
        path: Path,
        stack: tuple[Path, ...],
        errors: list[str],
    ) -> dict[str, JsonValue] | None:
        if path in stack:
            cycle = " -> ".join(self._display_path(root, item) for item in (*stack, path))
            errors.append(f"{QUALITY_MANIFEST_PATH}: include cycle detected: {cycle}")
            return None
        if not path.exists():
            errors.append(f"missing required quality manifest: {self._display_path(root, path)}")
            return None
        payload, error = self._json.load(path)
        if error is not None:
            errors.append(f"failed to parse {self._display_path(root, path)}: {error}")
            return None
        if not isinstance(payload, dict):
            errors.append(f"{self._display_path(root, path)} root must be an object")
            return None

        include_refs = self._read_includes(root, path, payload.get("includes"), errors)
        merged: dict[str, JsonValue] = {}
        child_stack = (*stack, path)
        for include_ref in include_refs:
            include_path = self._resolve_include_path(root, path, include_ref)
            if include_path is None:
                errors.append(f"{self._display_path(root, path)}: include path is invalid: {include_ref}")
                continue
            included = self._load_manifest_object(root, include_path, child_stack, errors)
            if included is None:
                continue
            self._merge_top_level(
                merged,
                included,
                errors,
                source=self._display_path(root, include_path),
            )

        local_payload: dict[str, JsonValue] = {}
        for key, value in payload.items():
            if key == "includes":
                continue
            local_payload[key] = value
        self._merge_top_level(
            merged,
            local_payload,
            errors,
            source=self._display_path(root, path),
        )
        return merged

    @staticmethod
    def _read_includes(
        root: Path,
        source_path: Path,
        value: JsonValue | None,
        errors: list[str],
    ) -> list[str]:
        if value is None:
            return []
        if not isinstance(value, list):
            errors.append(f"{QualityManifestLoader._display_path(root, source_path)}: 'includes' must be a list")
            return []
        refs: list[str] = []
        for item in value:
            if not isinstance(item, str) or not item.strip():
                errors.append(
                    f"{QualityManifestLoader._display_path(root, source_path)}: includes entries must be non-empty strings"
                )
                continue
            refs.append(item.strip())
        return refs

    @staticmethod
    def _resolve_include_path(root: Path, source_path: Path, include_ref: str) -> Path | None:
        include_path = Path(include_ref)
        if not include_path.is_absolute():
            include_path = (source_path.parent / include_path).resolve()
        else:
            include_path = include_path.resolve()
        try:
            include_path.relative_to(root)
        except ValueError:
            return None
        return include_path

    @staticmethod
    def _merge_top_level(
        target: dict[str, JsonValue],
        incoming: dict[str, JsonValue],
        errors: list[str],
        source: str,
    ) -> None:
        for key, value in incoming.items():
            if key in target:
                errors.append(f"{QUALITY_MANIFEST_PATH}: duplicate top-level key '{key}' while merging {source}")
                continue
            target[key] = value

    @staticmethod
    def _display_path(root: Path, path: Path) -> str:
        try:
            return path.resolve().relative_to(root).as_posix()
        except ValueError:
            return path.as_posix()

    @staticmethod
    def get_list(
        payload: dict[str, JsonValue],
        key: str,
        result: CheckResult,
        keyed_field: str = "id",
    ) -> list[dict[str, JsonValue]]:
        raw = payload.get(key)
        if isinstance(raw, dict):
            return QualityManifestLoader._coerce_dict_rows(raw, key, keyed_field, result)
        if not isinstance(raw, list):
            result.add_error(f"{QUALITY_MANIFEST_PATH}: '{key}' must be a list or object")
            return []
        rows: list[dict[str, JsonValue]] = []
        for item in raw:
            if not isinstance(item, dict):
                result.add_error(f"{QUALITY_MANIFEST_PATH}: '{key}' entries must be objects")
                continue
            rows.append(item)
        return rows

    @staticmethod
    def _coerce_dict_rows(
        raw: dict[str, JsonValue],
        key: str,
        keyed_field: str,
        result: CheckResult,
    ) -> list[dict[str, JsonValue]]:
        rows: list[dict[str, JsonValue]] = []
        for entry_key, entry_value in raw.items():
            if not isinstance(entry_value, dict):
                result.add_error(f"{QUALITY_MANIFEST_PATH}: '{key}' object entries must be objects")
                continue
            row = dict(entry_value)
            if keyed_field and keyed_field not in row:
                row[keyed_field] = entry_key
            rows.append(row)
        return rows

