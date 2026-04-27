#!/usr/bin/env python3
# @file python_tools/ci/release_quality_index.py
# @author Luis1454
# @project BLITZAR
# @brief Python quality and automation support for BLITZAR governance.

from __future__ import annotations

import json
import shutil
from collections.abc import Mapping, Sequence
from datetime import UTC, datetime
from pathlib import Path

from python_tools.ci.release_support import resolve_release_tag
from python_tools.policies.deviation_register import DeviationRegister
from python_tools.policies.quality_manifest import EvidenceRegistry, QualityManifestLoader

DEFAULT_INDEX_EVIDENCE_REFS = (
    "EVD_CI_RELEASE_LANE",
    "EVD_QLT_DEVIATION_REGISTER",
    "EVD_QLT_MANIFEST",
    "EVD_QLT_README",
    "EVD_QLT_RELEASE_INDEX_FORMAT",
    "EVD_QLT_STANDARDS_PROFILE",
    "EVD_SCRIPT_RELEASE_PACKAGE_INDEX",
)


# @brief Defines the release quality index error type contract.
# @param None This contract does not take explicit parameters.
# @note Keep construction and side effects explicit for deterministic quality gates.
class ReleaseQualityIndexError(RuntimeError):
    pass


# @brief Defines the release quality index builder type contract.
# @param None This contract does not take explicit parameters.
# @note Keep construction and side effects explicit for deterministic quality gates.
class ReleaseQualityIndexBuilder:
    # @brief Documents the init operation contract.
    # @param None This contract does not take explicit parameters.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def __init__(self) -> None:
        self._manifest = QualityManifestLoader()
        self._registry = EvidenceRegistry()
        self._deviations = DeviationRegister()

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
    ) -> Path:
        repo_root = root.resolve()
        manifest, errors = self._manifest.load_with_errors(repo_root)
        if errors:
            raise ReleaseQualityIndexError(errors[0])
        requirement_rows = self._select_requirements(manifest.get("requirements"), requirements)
        deviation_rows = self._select_deviations(repo_root, requirement_rows)
        index = self._build_index(repo_root, tag, profile, requirement_rows, deviation_rows, verification_activities, analyzer_status, ci_context)
        return self._archive_index(dist_dir.resolve(), tag, index)

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
            raise ReleaseQualityIndexError("quality manifest requirements payload is unavailable")
        selected_ids = sorted({item.strip() for item in selected or raw_requirements.keys() if item.strip()})
        rows: list[dict[str, object]] = []
        for requirement_id in selected_ids:
            row = raw_requirements.get(requirement_id)
            if not isinstance(row, dict):
                raise ReleaseQualityIndexError(f"unknown requirement id: {requirement_id}")
            rows.append(
                {
                    "id": requirement_id,
                    "evidence_refs": self._read_string_list(row, requirement_id, "artifacts"),
                }
            )
        return rows

    # @brief Documents the select deviations operation contract.
    # @param root Input value used by this contract.
    # @param requirements Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _select_deviations(
        self,
        root: Path,
        requirements: Sequence[Mapping[str, object]],
    ) -> list[dict[str, object]]:
        requirement_ids = {str(row["id"]) for row in requirements}
        result_rows, errors = self._deviations.load_open_with_errors(root)
        if errors:
            raise ReleaseQualityIndexError(errors[0])
        selected: list[dict[str, object]] = []
        for row in result_rows:
            linked_requirements = row.get("requirements")
            if not isinstance(linked_requirements, list):
                continue
            if any(isinstance(item, str) and item in requirement_ids for item in linked_requirements):
                selected.append(row)
        return selected

    # @brief Documents the build index operation contract.
    # @param root Input value used by this contract.
    # @param tag Input value used by this contract.
    # @param profile Input value used by this contract.
    # @param requirement_rows Input value used by this contract.
    # @param deviation_rows Input value used by this contract.
    # @param verification_activities Input value used by this contract.
    # @param analyzer_status Input value used by this contract.
    # @param ci_context Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _build_index(
        self,
        root: Path,
        tag: str,
        profile: str,
        requirement_rows: Sequence[Mapping[str, object]],
        deviation_rows: Sequence[Mapping[str, object]],
        verification_activities: Sequence[Mapping[str, str]] | None,
        analyzer_status: Mapping[str, str] | None,
        ci_context: Mapping[str, str] | None,
    ) -> dict[str, object]:
        requirement_ids = [str(row["id"]) for row in requirement_rows]
        evidence_refs = self._collect_evidence_refs(root, requirement_rows)
        return {
            "format_version": "1.0",
            "tag": tag,
            "profile": profile,
            "generated_at_utc": datetime.now(UTC).isoformat(timespec="seconds"),
            "entry_point": "release-quality-index",
            "summary": {
                "requirements": len(requirement_ids),
                "evidence_refs": len(evidence_refs),
                "open_deviations": len(deviation_rows),
                "verification_activities": len(list(verification_activities or ())),
            },
            "requirement_ids": requirement_ids,
            "evidence_refs": evidence_refs,
            "analyzers": [{"name": name, "status": status} for name, status in sorted((analyzer_status or {}).items())],
            "verification_activities": [dict(activity) for activity in verification_activities or ()],
            "open_deviations": [self._summarize_deviation(row) for row in deviation_rows],
            "ci_context": dict(ci_context or {}),
        }

    # @brief Documents the archive index operation contract.
    # @param dist_dir Input value used by this contract.
    # @param tag Input value used by this contract.
    # @param index Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _archive_index(self, dist_dir: Path, tag: str, index: Mapping[str, object]) -> Path:
        if dist_dir.exists():
            shutil.rmtree(dist_dir)
        dist_dir.mkdir(parents=True, exist_ok=True)
        (dist_dir / "release_quality_index.json").write_text(json.dumps(index, indent=2), encoding="utf-8")
        (dist_dir / "README.md").write_text(self._render_readme(index), encoding="utf-8")
        archive_base = dist_dir / f"ASTER-{tag}-quality-index"
        return Path(shutil.make_archive(str(archive_base), "zip", root_dir=dist_dir))

    # @brief Documents the collect evidence refs operation contract.
    # @param root Input value used by this contract.
    # @param requirements Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _collect_evidence_refs(self, root: Path, requirements: Sequence[Mapping[str, object]]) -> list[dict[str, str]]:
        refs = set(DEFAULT_INDEX_EVIDENCE_REFS)
        for row in requirements:
            values = row.get("evidence_refs")
            if isinstance(values, list):
                refs.update(str(item) for item in values)
        entries: list[dict[str, str]] = []
        for ref in sorted(refs):
            path_str, error = self._registry.resolve(root, ref)
            if error is not None or path_str is None:
                raise ReleaseQualityIndexError(error or f"failed to resolve evidence ref: {ref}")
            entries.append({"id": ref, "path": path_str})
        return entries

    @staticmethod
    # @brief Documents the summarize deviation operation contract.
    # @param row Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _summarize_deviation(row: Mapping[str, object]) -> dict[str, object]:
        return {
            "id": row.get("id", ""),
            "scope": row.get("scope", ""),
            "status": row.get("status", ""),
            "review_by": row.get("review_by", ""),
            "requirements": row.get("requirements", []),
        }

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
            raise ReleaseQualityIndexError(f"{requirement_id}: '{field}' must be a list")
        values: list[str] = []
        for item in raw:
            if not isinstance(item, str) or not item.strip():
                raise ReleaseQualityIndexError(f"{requirement_id}: '{field}' entries must be non-empty strings")
            values.append(item.strip())
        return values

    @staticmethod
    # @brief Documents the render readme operation contract.
    # @param index Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _render_readme(index: Mapping[str, object]) -> str:
        summary = index.get("summary")
        if not isinstance(summary, dict):
            summary = {}
        return (
            "# Release Quality Index\n\n"
            f"- Tag: `{index.get('tag', '')}`\n"
            f"- Profile: `{index.get('profile', '')}`\n"
            f"- Requirements: `{summary.get('requirements', 0)}`\n"
            f"- Evidence refs: `{summary.get('evidence_refs', 0)}`\n"
            f"- Open deviations: `{summary.get('open_deviations', 0)}`\n\n"
            "Use `release_quality_index.json` as the audit entry point for release review.\n"
        )
