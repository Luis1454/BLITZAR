#!/usr/bin/env python3
# File: python_tools/policies/quality_manifest.py
# Purpose: Python quality and automation support for BLITZAR governance.

from __future__ import annotations

from collections.abc import Mapping
from pathlib import Path
from types import MappingProxyType

from python_tools.core.io import JsonLoader
from python_tools.core.models import CheckResult, JsonValue

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
        return ({}, errors) if payload is None else (payload, errors)

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
        include_path = include_path.resolve() if include_path.is_absolute() else (source_path.parent / include_path).resolve()
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
        rows = [item for item in raw if isinstance(item, dict)]
        invalid_count = len(raw) - len(rows)
        for _ in range(invalid_count):
            result.add_error(f"{QUALITY_MANIFEST_PATH}: '{key}' entries must be objects")
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
            row = {keyed_field: entry_key, **entry_value} if keyed_field and keyed_field not in entry_value else dict(entry_value)
            rows.append(row)
        return rows


class EvidenceRegistry:
    def __init__(self) -> None:
        self._manifest = QualityManifestLoader()
        self._cache_by_root: dict[Path, tuple[Mapping[str, str], Mapping[str, str]]] = {}

    def resolve(self, root: Path, ref: str) -> tuple[str | None, str | None]:
        by_id, _by_path, load_error = self._load(root)
        if load_error is not None:
            return None, load_error
        if by_id is None:
            return None, "quality evidence registry is unavailable"
        if ref in by_id:
            return by_id[ref], None
        if "/" in ref or "\\" in ref:
            return None, f"path refs are not allowed in quality manifest: {ref} (use EVD_* id)"
        return None, f"unknown evidence reference id: {ref}"

    def normalize(self, root: Path, value: str) -> str:
        _by_id, by_path, load_error = self._load(root)
        if load_error is not None or by_path is None:
            return value
        if value in by_path:
            return by_path[value]
        return value

    def _load(self, root: Path) -> tuple[Mapping[str, str] | None, Mapping[str, str] | None, str | None]:
        if root in self._cache_by_root:
            by_id, by_path = self._cache_by_root[root]
            return by_id, by_path, None
        payload, errors = self._manifest.load_with_errors(root)
        if errors:
            return None, None, errors[0]
        evidence_raw = payload.get("evidence")
        if not isinstance(evidence_raw, dict):
            return None, None, f"{QUALITY_MANIFEST_PATH}: 'evidence' must be an object"
        evidence_map: dict[str, str] = {}
        for ref, path in evidence_raw.items():
            if not isinstance(ref, str) or not ref.strip():
                return None, None, f"{QUALITY_MANIFEST_PATH}: evidence keys must be non-empty strings"
            if not isinstance(path, str) or not path.strip():
                return None, None, f"{QUALITY_MANIFEST_PATH}: evidence path for '{ref}' must be non-empty string"
            evidence_map[ref.strip()] = path.strip()
        by_id = MappingProxyType(evidence_map)
        by_path = MappingProxyType({path: ref for ref, path in evidence_map.items()})
        self._cache_by_root[root] = (by_id, by_path)
        return by_id, by_path, None


_REGISTRY = EvidenceRegistry()


def resolve_evidence_ref(root: Path, ref: str) -> tuple[str | None, str | None]:
    return _REGISTRY.resolve(root, ref)
