#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path

from python_tools.core.base_check import BaseCheck
from python_tools.core.io import REQ_ID_RE, TEST_MACRO_RE, GitTrackedService, collect_test_ids
from python_tools.core.models import CheckContext, CheckResult
from python_tools.core.typing_ext import JsonValue
from python_tools.policies.deviation_register import DeviationRegister
from python_tools.policies.evidence_registry import resolve_evidence_ref
from python_tools.policies.quality_manifest import CROSSWALK_KEY, QUALITY_MANIFEST_PATH, REQUIREMENTS_KEY, QualityManifestLoader
from python_tools.policies.repo_policy import load_allowlist
from python_tools.policies.test_id_filter import collect_repo_quality_test_ids

REQUIRED_FILES = (
    "AGENTS.md",
    "docs/quality/README.md",
    QUALITY_MANIFEST_PATH,
    "docs/quality/standards_profile.md",
    "docs/quality/fmea.md",
    "docs/quality/tool_qualification.md",
    "docs/quality/tool_manifest.md",
    "docs/quality/prod_baseline.md",
    "docs/quality/release_index.md",
    "docs/quality/interface_contracts.md",
    "docs/quality/ivv_plan.md",
    "docs/quality/numerical_validation.md",
)
REQUIRED_EVIDENCE = {"EVD_AGENTS": "AGENTS.md"}
REQUIRED_CROSSWALK_ARTIFACTS = {"SWE-004": "EVD_AGENTS"}
SUPPORTED_STANDARDS = {"NPR-7150.2D", "NASA-STD-8739.8B", "ECSS-E-ST-40C", "ECSS-Q-ST-80C", "ECSS-E-ST-40-07C"}


class QualityBaselineCheck(BaseCheck):
    name = "quality"
    success_message = "quality baseline check passed"
    failure_title = "quality baseline check failed:"

    def __init__(self) -> None:
        self._manifest = QualityManifestLoader()
        self._deviations = DeviationRegister()
        self._git = GitTrackedService()

    def _execute(self, context: CheckContext, result: CheckResult) -> None:
        self._ensure_required_files(context.root, result)
        manifest = self._manifest.load(context.root, result)
        requirements = self._load_requirements(manifest, result)
        override = context.options.get("extra_test_ids")
        extra_ids = (
            override
            if isinstance(override, set)
            else collect_repo_quality_test_ids(context.root, manifest, result)
        )
        tests = collect_test_ids(context.root, extra_ids, TEST_MACRO_RE)
        self._check_requirement_tests(requirements, tests, result)
        self._check_requirement_artifacts(context.root, requirements, result)
        self._check_deviations(context.root, manifest, requirements, result)
        self._check_required_evidence(context.root, manifest, result)
        self._check_crosswalk(context.root, manifest, result)

    def _ensure_required_files(self, root: Path, result: CheckResult) -> None:
        for rel in REQUIRED_FILES:
            path = root / rel
            if not path.exists():
                result.add_error(f"missing required quality file: {rel}")
                continue
            if path.stat().st_size == 0:
                result.add_error(f"quality file is empty: {rel}")

    def _load_requirements(self, manifest: dict[str, JsonValue], result: CheckResult) -> dict[str, dict[str, list[str]]]:
        rows = self._manifest.get_list(manifest, REQUIREMENTS_KEY, result, keyed_field="id")
        parsed: dict[str, dict[str, list[str]]] = {}
        for row in rows:
            req_id = self._as_string(row.get("id"))
            if not REQ_ID_RE.match(req_id):
                result.add_error(f"invalid requirement id format: {req_id}")
                continue
            if req_id in parsed:
                result.add_error(f"duplicate requirement id: {req_id}")
                continue
            tests = self._as_string_list(row.get("tests"))
            artifacts = self._as_string_list(row.get("artifacts"))
            if not tests:
                result.add_error(f"{req_id}: missing tests list")
            if not artifacts:
                result.add_error(f"{req_id}: missing artifacts list")
            parsed[req_id] = {"tests": tests, "artifacts": artifacts}
        return parsed

    def _check_requirement_tests(self, requirements: dict[str, dict[str, list[str]]], tests: set[str], result: CheckResult) -> None:
        for req_id, req in requirements.items():
            for pattern in req["tests"]:
                try:
                    regex = re.compile(pattern)
                except re.error as exc:
                    result.add_error(f"{req_id}: invalid test regex '{pattern}': {exc}")
                    continue
                if not any(regex.search(test_id) for test_id in tests):
                    result.add_error(f"{req_id}: test regex did not match any test id: {pattern}")

    def _check_requirement_artifacts(self, root: Path, requirements: dict[str, dict[str, list[str]]], result: CheckResult) -> None:
        for req_id, req in requirements.items():
            for artifact_ref in req["artifacts"]:
                artifact_path, artifact_error = resolve_evidence_ref(root, artifact_ref)
                if artifact_error is not None:
                    result.add_error(f"{req_id}: {artifact_error}")
                    continue
                if artifact_path is None:
                    result.add_error(f"{req_id}: unresolved artifact ref: {artifact_ref}")
                    continue
                if not (root / artifact_path).exists():
                    result.add_error(f"{req_id}: referenced artifact does not exist: {artifact_ref} -> {artifact_path}")

    def _check_deviations(
        self,
        root: Path,
        manifest: dict[str, JsonValue],
        requirements: dict[str, dict[str, list[str]]],
        result: CheckResult,
    ) -> None:
        rows = self._deviations.load(root, manifest, set(requirements), result)
        allowlist = load_allowlist(root / "tests/checks/policy_allowlist.txt")
        registered_paths: set[str] = set()
        for row in rows:
            if row.get("status") != "open":
                continue
            paths = row.get("paths")
            if isinstance(paths, list):
                registered_paths.update(path for path in paths if isinstance(path, str))
        for path in sorted(allowlist - registered_paths):
            result.add_error(f"policy allowlist path is missing from deviation register: {path}")

    def _check_required_evidence(self, root: Path, manifest: dict[str, JsonValue], result: CheckResult) -> None:
        evidence = manifest.get("evidence")
        if not isinstance(evidence, dict):
            result.add_error(f"{QUALITY_MANIFEST_PATH}: 'evidence' must be an object")
            return
        for artifact_id, expected_path in REQUIRED_EVIDENCE.items():
            actual_path = evidence.get(artifact_id)
            if not actual_path:
                result.add_error(f"{QUALITY_MANIFEST_PATH}: missing required evidence id: {artifact_id}")
                continue
            if not isinstance(actual_path, str) or actual_path.strip() != expected_path:
                result.add_error(f"{QUALITY_MANIFEST_PATH}: {artifact_id} must map to {expected_path}, got {actual_path}")
                continue
            if not self._git.is_tracked(root, expected_path):
                result.add_error(f"{QUALITY_MANIFEST_PATH}: required evidence must be git-tracked: {artifact_id} -> {expected_path}")

    def _check_crosswalk(self, root: Path, manifest: dict[str, JsonValue], result: CheckResult) -> None:
        rows = self._manifest.get_list(manifest, CROSSWALK_KEY, result, keyed_field="control_id")
        if not rows:
            result.add_error(f"{QUALITY_MANIFEST_PATH}: '{CROSSWALK_KEY}' must contain at least one row")
            return
        seen_controls: dict[str, str] = {}
        for row in rows:
            control_id = self._as_string(row.get("control_id"))
            source_standard = self._as_string(row.get("source_standard"))
            repo_artifact_ref = self._as_string(row.get("artifact"))
            check = self._as_string(row.get("check"))
            if control_id:
                seen_controls[control_id] = repo_artifact_ref
            if not control_id:
                result.add_error(f"{QUALITY_MANIFEST_PATH}: crosswalk row has empty id")
            if source_standard not in SUPPORTED_STANDARDS:
                result.add_error(f"{QUALITY_MANIFEST_PATH}: unsupported source_standard '{source_standard}'")
            if not check:
                result.add_error(f"{control_id or '<missing control>'}: missing check field")
            if not repo_artifact_ref:
                result.add_error(f"{control_id or '<missing control>'}: missing artifact")
                continue
            repo_artifact_path, artifact_error = resolve_evidence_ref(root, repo_artifact_ref)
            if artifact_error is not None:
                result.add_error(f"{control_id or '<missing control>'}: {artifact_error}")
                continue
            if repo_artifact_path is None:
                result.add_error(f"{control_id or '<missing control>'}: unresolved artifact ref: {repo_artifact_ref}")
                continue
            if not (root / repo_artifact_path).exists():
                result.add_error(
                    f"{control_id or '<missing control>'}: artifact does not exist: {repo_artifact_ref} -> {repo_artifact_path}"
                )
                continue
            if repo_artifact_path == "AGENTS.md" and not self._git.is_tracked(root, repo_artifact_path):
                result.add_error(f"{control_id or '<missing control>'}: artifact must be git-tracked: {repo_artifact_ref}")
        for control_id, artifact_ref in REQUIRED_CROSSWALK_ARTIFACTS.items():
            if seen_controls.get(control_id) != artifact_ref:
                result.add_error(f"{QUALITY_MANIFEST_PATH}: crosswalk control {control_id} must reference {artifact_ref}")

    @staticmethod
    def _as_string(value: JsonValue | None) -> str:
        return value.strip() if isinstance(value, str) else ""

    @staticmethod
    def _as_string_list(value: JsonValue | None) -> list[str]:
        if not isinstance(value, list):
            return []
        return [item.strip() for item in value if isinstance(item, str) and item.strip()]
