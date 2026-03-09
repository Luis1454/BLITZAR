#!/usr/bin/env python3
from __future__ import annotations

from collections.abc import Mapping
from pathlib import Path
from types import MappingProxyType

from python_tools.checks.quality_manifest import QUALITY_MANIFEST_PATH, QualityManifestLoader


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

    def _load(
        self,
        root: Path,
    ) -> tuple[Mapping[str, str] | None, Mapping[str, str] | None, str | None]:
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


def normalize_evidence_ref(root: Path, value: str) -> str:
    return _REGISTRY.normalize(root, value)

