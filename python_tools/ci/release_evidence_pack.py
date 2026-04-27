#!/usr/bin/env python3
# @file python_tools/ci/release_evidence_pack.py
# @author Luis1454
# @project BLITZAR
# @brief Python quality and automation support for BLITZAR governance.

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


# @brief Defines the release evidence pack error type contract.
# @param None This contract does not take explicit parameters.
# @note Keep construction and side effects explicit for deterministic quality gates.
class ReleaseEvidencePackError(RuntimeError):
    pass


# @brief Defines the release evidence packager type contract.
# @param None This contract does not take explicit parameters.
# @note Keep construction and side effects explicit for deterministic quality gates.
class ReleaseEvidencePackager:
    # @brief Documents the init operation contract.
    # @param None This contract does not take explicit parameters.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def __init__(self) -> None:
        self._manifest = QualityManifestLoader()
        self._registry = EvidenceRegistry()

    # @brief Documents the resolve tag operation contract.
    # @param explicit Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def resolve_tag(self, explicit: str | None) -> str:
        return resolve_release_tag(explicit)

    # @brief Documents the package operation contract.
    # @param root Input value used by this contract.
    # @param dist_dir Input value used by this contract.
    # @param tag Input value used by this contract.
    # @param profile Input value used by this contract.
    # @param requirements Input value used by this contract.
    # @param verification_activities Input value used by this contract.
    # @param analyzer_status Input value used by this contract.
    # @param ci_context Input value used by this contract.
    # @param extra_evidence_refs Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
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

    # @brief Documents the select requirements operation contract.
    # @param raw_requirements Input value used by this contract.
    # @param selected Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
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

    # @brief Documents the resolve evidence files operation contract.
    # @param root Input value used by this contract.
    # @param requirement_rows Input value used by this contract.
    # @param extra_evidence_refs Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
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

    # @brief Documents the build pack operation contract.
    # @param root Input value used by this contract.
    # @param tag Input value used by this contract.
    # @param profile Input value used by this contract.
    # @param requirement_rows Input value used by this contract.
    # @param evidence_files Input value used by this contract.
    # @param verification_activities Input value used by this contract.
    # @param analyzer_status Input value used by this contract.
    # @param ci_context Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
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

    # @brief Documents the archive pack operation contract.
    # @param root Input value used by this contract.
    # @param dist_dir Input value used by this contract.
    # @param tag Input value used by this contract.
    # @param pack Input value used by this contract.
    # @param evidence_files Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
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
            if source.is_dir():
                shutil.copytree(source, destination, dirs_exist_ok=True)
            else:
                shutil.copy2(source, destination)
        archive_base = dist_dir / f"ASTER-{tag}-evidence"
        return Path(shutil.make_archive(str(archive_base), "zip", root_dir=dist_dir))

    @staticmethod
    # @brief Documents the read string list operation contract.
    # @param row Input value used by this contract.
    # @param requirement_id Input value used by this contract.
    # @param field Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
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
