#!/usr/bin/env python3
from __future__ import annotations

from python_tools.core.base_check import BaseCheck
from python_tools.core.io import JsonValue
from python_tools.core.models import CheckContext, CheckResult
from python_tools.policies.quality_manifest import QUALITY_MANIFEST_PATH, QualityManifestLoader


class MirrorCheck(BaseCheck):
    name = "mirror"
    success_message = "Header/CPP mirror validation passed"
    failure_title = "Header/CPP mirror validation failed:"

    def __init__(self) -> None:
        self._manifest = QualityManifestLoader()

    def _execute(self, context: CheckContext, result: CheckResult) -> None:
        mirror_pairs, header_only = self._load_config(context, result)
        if not mirror_pairs:
            return
        for hdr_rel, src_rel in mirror_pairs:
            hdr_dir = context.root / hdr_rel
            src_dir = context.root / src_rel
            if not hdr_dir.exists():
                continue
            headers = sorted(hdr_dir.glob("*.hpp"))
            if not src_dir.exists():
                if self._all_headers_header_only(hdr_rel, headers, header_only):
                    continue
                result.add_error(f"Missing source mirror directory: {src_rel}")
                continue
            self._check_pair(hdr_rel, src_rel, headers, src_dir, header_only, result)

    def _check_pair(
        self,
        hdr_rel: str,
        src_rel: str,
        headers,
        src_dir,
        header_only: set[str],
        result: CheckResult,
    ) -> None:
        sources = sorted(src_dir.glob("*.cpp"))
        header_bases = {h.stem for h in headers}
        for header in headers:
            base = header.stem
            header_rel = f"{hdr_rel}/{base}.hpp"
            if header_rel not in header_only and not (src_dir / f"{base}.cpp").exists():
                result.add_error(f"Missing cpp for {header_rel} -> expected {src_rel}/{base}.cpp")
        for source in sources:
            base = source.stem
            if base not in header_bases:
                result.add_error(f"Missing hpp for {src_rel}/{base}.cpp -> expected {hdr_rel}/{base}.hpp")

    def _all_headers_header_only(self, hdr_rel: str, headers, header_only: set[str]) -> bool:
        if not headers:
            return True
        for header in headers:
            header_rel = f"{hdr_rel}/{header.stem}.hpp"
            if header_rel not in header_only:
                return False
        return True

    def _load_config(self, context: CheckContext, result: CheckResult) -> tuple[list[tuple[str, str]], set[str]]:
        manifest = self._manifest.load(context.root, result)
        policies = manifest.get("policies")
        if not isinstance(policies, dict):
            result.add_error(f"{QUALITY_MANIFEST_PATH}: 'policies' must be an object")
            return [], set()
        mirror = policies.get("mirror")
        if not isinstance(mirror, dict):
            result.add_error(f"{QUALITY_MANIFEST_PATH}: 'policies.mirror' must be an object")
            return [], set()
        pairs = self._read_pairs(mirror.get("pairs"), result)
        header_only = self._read_header_only(mirror.get("header_only"), result)
        return pairs, header_only

    def _read_pairs(self, value: JsonValue | None, result: CheckResult) -> list[tuple[str, str]]:
        if not isinstance(value, dict):
            result.add_error(f"{QUALITY_MANIFEST_PATH}: 'policies.mirror.pairs' must be an object")
            return []
        pairs: list[tuple[str, str]] = []
        for hdr_rel, src_rel in value.items():
            if not isinstance(hdr_rel, str) or not isinstance(src_rel, str) or not hdr_rel or not src_rel:
                result.add_error(f"{QUALITY_MANIFEST_PATH}: mirror pair entries must be non-empty strings")
                continue
            pairs.append((hdr_rel.strip(), src_rel.strip()))
        return pairs

    def _read_header_only(self, value: JsonValue | None, result: CheckResult) -> set[str]:
        if not isinstance(value, list):
            result.add_error(f"{QUALITY_MANIFEST_PATH}: 'policies.mirror.header_only' must be a list")
            return set()
        paths: set[str] = set()
        for item in value:
            if not isinstance(item, str) or not item.strip():
                result.add_error(f"{QUALITY_MANIFEST_PATH}: header_only entries must be non-empty strings")
                continue
            paths.add(item.strip())
        return paths
