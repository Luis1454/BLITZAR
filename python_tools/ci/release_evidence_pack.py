#!/usr/bin/env python3
from __future__ import annotations

import json
import shutil
from collections.abc import Mapping, Sequence
from datetime import UTC, datetime
from pathlib import Path

from python_tools.ci.release_support import (
    DEFAULT_EVIDENCE_REFS,
    load_open_exceptions,
    render_pack_readme,
    resolve_release_tag,
)
from python_tools.policies.quality_manifest import EvidenceRegistry, QualityManifestLoader


class ReleaseEvidencePackError(RuntimeError):
    pass


class ReleaseEvidencePackager:
    def __init__(self) -> None:
        self._manifest = QualityManifestLoader()
        self._registry = EvidenceRegistry()

    def resolve_tag(self, explicit: str | None) -> str:
        return resolve_release_tag(explicit)

    def package(
        self,
        root: Path,
        dist_dir: Path,
        tag: str,
        profile: str,
        requirements: Sequence[str] | None = None,
        verification_activities: Sequence[Mapping[str, str]] | None = None,
        analyzer_status: Mapping[str, str] | None = None,
        ci_context: Mapping[str, str] | None = None,
        extra_evidence_refs: Sequence[str] | None = None,
    ) -> Path:
        repo_root = root.resolve()
        payload, errors = self._manifest.load_with_errors(repo_root)
        if errors:
            raise ReleaseEvidencePackError(errors[0])
        requirement_rows = self._select_requirements(payload.get("requirements"), requirements)
        evidence_files = self._resolve_evidence_files(repo_root, requirement_rows, extra_evidence_refs or ())
        pack = self._build_pack(
            repo_root,
            tag,
            profile,
            requirement_rows,
            evidence_files,
            verification_activities,
            analyzer_status,
            ci_context,
        )
        return self._archive_pack(repo_root, dist_dir.resolve(), tag, pack, evidence_files)

    def _select_requirements(
        self,
        raw_requirements: object,
        selected: Sequence[str] | None,
    ) -> list[dict[str, object]]:
        if not isinstance(raw_requirements, dict):
            raise ReleaseEvidencePackError("quality manifest requirements payload is unavailable")
        selected_ids = sorted({item.strip() for item in selected or raw_requirements.keys() if item.strip()})
        rows: list[dict[str, object]] = []
        for requirement_id in selected_ids:
            row = raw_requirements.get(requirement_id)
            if not isinstance(row, dict):
                raise ReleaseEvidencePackError(f"unknown requirement id: {requirement_id}")
            rows.append(
                {
                    "id": requirement_id,
                    "tests": self._read_string_list(row, requirement_id, "tests"),
                    "evidence_refs": self._read_string_list(row, requirement_id, "artifacts"),
                }
            )
        return rows

    def _resolve_evidence_files(
        self,
        root: Path,
        requirement_rows: Sequence[Mapping[str, object]],
        extra_evidence_refs: Sequence[str],
    ) -> list[dict[str, str]]:
        refs = {ref for ref in DEFAULT_EVIDENCE_REFS}
        refs.update(ref for ref in extra_evidence_refs if ref.strip())
        for row in requirement_rows:
            artifacts = row.get("evidence_refs")
            if isinstance(artifacts, list):
                refs.update(str(ref) for ref in artifacts)
        files: list[dict[str, str]] = []
        for ref in sorted(refs):
            path_str, error = self._registry.resolve(root, ref)
            if error is not None or path_str is None:
                raise ReleaseEvidencePackError(error or f"failed to resolve evidence ref: {ref}")
            source = (root / path_str).resolve()
            if not source.exists():
                raise ReleaseEvidencePackError(f"missing evidence file for {ref}: {path_str}")
            files.append({"id": ref, "path": path_str})
        return files

    def _build_pack(
        self,
        root: Path,
        tag: str,
        profile: str,
        requirement_rows: Sequence[Mapping[str, object]],
        evidence_files: Sequence[Mapping[str, str]],
        verification_activities: Sequence[Mapping[str, str]] | None,
        analyzer_status: Mapping[str, str] | None,
        ci_context: Mapping[str, str] | None,
    ) -> dict[str, object]:
        return {
            "format_version": "1.0",
            "tag": tag,
            "profile": profile,
            "generated_at_utc": datetime.now(UTC).isoformat(timespec="seconds"),
            "requirement_ids": [str(row["id"]) for row in requirement_rows],
            "requirements": [dict(row) for row in requirement_rows],
            "verification_activities": [dict(activity) for activity in verification_activities or ()],
            "analyzers": [{"name": name, "status": status} for name, status in sorted((analyzer_status or {}).items())],
            "ci_context": dict(ci_context or {}),
            "evidence_refs": [dict(item) for item in evidence_files],
            "open_exceptions": load_open_exceptions(root),
        }

    def _archive_pack(
        self,
        root: Path,
        dist_dir: Path,
        tag: str,
        pack: Mapping[str, object],
        evidence_files: Sequence[Mapping[str, str]],
    ) -> Path:
        if dist_dir.exists():
            shutil.rmtree(dist_dir)
        dist_dir.mkdir(parents=True, exist_ok=True)
        (dist_dir / "release_evidence_pack.json").write_text(json.dumps(pack, indent=2), encoding="utf-8")
        (dist_dir / "README.md").write_text(render_pack_readme(pack), encoding="utf-8")
        evidence_root = dist_dir / "evidence"
        for item in evidence_files:
            source = root / item["path"]
            destination = evidence_root / item["path"]
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source, destination)
        archive_base = dist_dir / f"CUDA-GRAVITY-SIMULATION-{tag}-evidence"
        return Path(shutil.make_archive(str(archive_base), "zip", root_dir=dist_dir))

    @staticmethod
    def _read_string_list(row: Mapping[str, object], requirement_id: str, field: str) -> list[str]:
        raw = row.get(field)
        if not isinstance(raw, list):
            raise ReleaseEvidencePackError(f"{requirement_id}: '{field}' must be a list")
        values: list[str] = []
        for item in raw:
            if not isinstance(item, str) or not item.strip():
                raise ReleaseEvidencePackError(f"{requirement_id}: '{field}' entries must be non-empty strings")
            values.append(item.strip())
        return values
