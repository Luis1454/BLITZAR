#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path

from python_tools.core.base_check import BaseCheck
from python_tools.core.io import REQ_ID_RE, TEST_MACRO_RE, CsvLoader, GitTrackedService, JsonLoader, collect_test_ids
from python_tools.core.models import CheckContext, CheckResult

REQUIRED_FILES = (
    "docs/quality/README.md",
    "docs/quality/requirements.json",
    "docs/quality/traceability.csv",
    "docs/quality/test_catalog.csv",
    "docs/quality/standards_profile.md",
    "docs/quality/nasa_crosswalk.csv",
    "docs/quality/fmea.md",
    "docs/quality/tool_qualification.md",
    "docs/quality/ivv_plan.md",
    "docs/quality/numerical_validation.md",
)
REQUIRED_COLUMNS = ("req_id", "item_type", "item_ref", "evidence")
SUPPORTED_ITEM_TYPES = {"test_regex", "source", "doc", "analysis"}
CROSSWALK_COLUMNS = ("control_id", "source_standard", "repo_artifact", "verification")
SUPPORTED_STANDARDS = {"NPR-7150.2D", "NASA-STD-8739.8B", "ECSS-E-ST-40C", "ECSS-Q-ST-80C", "ECSS-E-ST-40-07C"}
EXTRA_TEST_IDS = {
    "TST_QLT_REPO_001_GravityIniCheck",
    "TST_QLT_REPO_002_GravityMirrorCheck",
    "TST_QLT_REPO_003_GravityNoLegacyCheck",
    "TST_QLT_REPO_004_GravityRepoPolicyCheck",
    "TST_QLT_REPO_005_GravityLauncherCheck",
    "TST_QLT_REPO_006_GravityQualityBaselineCheck",
    "TST_QLT_REPO_007_PrPolicyCheck",
    "TST_QLT_REPO_008_PyChecksUnit",
    "TST_QLT_REPO_009_PythonQualityGate",
}


class QualityBaselineCheck(BaseCheck):
    name = "quality"
    success_message = "quality baseline check passed"
    failure_title = "quality baseline check failed:"

    def __init__(self) -> None:
        self._json = JsonLoader()
        self._csv = CsvLoader()
        self._git = GitTrackedService()

    def _execute(self, context: CheckContext, result: CheckResult) -> None:
        self._ensure_required_files(context.root, result)
        requirements = self._load_requirements(context.root, result)
        override = context.options.get("extra_test_ids")
        extra_ids = override if isinstance(override, set) else EXTRA_TEST_IDS
        tests = collect_test_ids(context.root, extra_ids, TEST_MACRO_RE)
        self._check_requirement_tests(requirements, tests, result)
        self._check_traceability(context.root, requirements, tests, result)
        self._check_crosswalk(context.root, result)

    def _ensure_required_files(self, root: Path, result: CheckResult) -> None:
        for rel in REQUIRED_FILES:
            path = root / rel
            if not path.exists():
                result.add_error(f"missing required quality file: {rel}")
                continue
            if path.stat().st_size == 0:
                result.add_error(f"quality file is empty: {rel}")

    def _load_requirements(self, root: Path, result: CheckResult) -> dict[str, dict]:
        payload, error = self._json.load(root / "docs/quality/requirements.json")
        if error is not None:
            result.add_error(f"failed to parse docs/quality/requirements.json: {error}")
            return {}
        if not isinstance(payload, dict):
            result.add_error("requirements.json root must be an object")
            return {}
        requirements = payload.get("requirements")
        if not isinstance(requirements, list):
            result.add_error("requirements.json: 'requirements' must be a list")
            return {}
        parsed: dict[str, dict] = {}
        for req in requirements:
            if not isinstance(req, dict):
                result.add_error("requirements.json: each requirement must be an object")
                continue
            req_id = req.get("id", "")
            if not isinstance(req_id, str) or not REQ_ID_RE.match(req_id):
                result.add_error(f"invalid requirement id format: {req_id}")
                continue
            if req_id in parsed:
                result.add_error(f"duplicate requirement id: {req_id}")
                continue
            title = req.get("title", "")
            verification = req.get("verification", {})
            tests: list[str] = []
            if isinstance(verification, dict):
                candidate = verification.get("tests", [])
                if isinstance(candidate, list):
                    tests = [str(item) for item in candidate]
            if not isinstance(title, str) or not title.strip():
                result.add_error(f"{req_id}: missing title")
            if not tests:
                result.add_error(f"{req_id}: missing verification tests list")
            parsed[req_id] = req
        return parsed

    def _check_requirement_tests(self, requirements: dict[str, dict], tests: set[str], result: CheckResult) -> None:
        for req_id, req in requirements.items():
            patterns = req.get("verification", {}).get("tests", [])
            for pattern in patterns:
                try:
                    regex = re.compile(pattern)
                except re.error as exc:
                    result.add_error(f"{req_id}: invalid test regex '{pattern}': {exc}")
                    continue
                if not any(regex.search(test_id) for test_id in tests):
                    result.add_error(f"{req_id}: test regex did not match any test id: {pattern}")

    def _check_traceability(self, root: Path, requirements: dict[str, dict], tests: set[str], result: CheckResult) -> None:
        rows, errors = self._csv.load_rows(
            root / "docs/quality/traceability.csv",
            REQUIRED_COLUMNS,
            "docs/quality/traceability.csv",
            "traceability.csv columns must be exactly: ",
        )
        for error in errors:
            result.add_error(error)
        covered: set[str] = set()
        for row in rows:
            req_id = (row.get("req_id") or "").strip()
            item_type = (row.get("item_type") or "").strip()
            item_ref = (row.get("item_ref") or "").strip()
            if req_id not in requirements:
                result.add_error(f"traceability references unknown requirement id: {req_id}")
                continue
            covered.add(req_id)
            if item_type not in SUPPORTED_ITEM_TYPES:
                result.add_error(f"{req_id}: unsupported item_type '{item_type}'")
                continue
            if item_type in {"source", "doc"} and not (root / item_ref).exists():
                result.add_error(f"{req_id}: referenced path does not exist: {item_ref}")
            if item_type == "test_regex":
                self._check_test_regex(req_id, item_ref, tests, result)
        for req_id in sorted(set(requirements.keys()) - covered):
            result.add_error(f"requirement has no traceability row: {req_id}")

    def _check_test_regex(self, req_id: str, pattern: str, tests: set[str], result: CheckResult) -> None:
        try:
            regex = re.compile(pattern)
        except re.error as exc:
            result.add_error(f"{req_id}: invalid test regex in traceability: {pattern} ({exc})")
            return
        if not any(regex.search(test_id) for test_id in tests):
            result.add_error(f"{req_id}: traceability regex did not match any test id: {pattern}")

    def _check_crosswalk(self, root: Path, result: CheckResult) -> None:
        rows, errors = self._csv.load_rows(
            root / "docs/quality/nasa_crosswalk.csv",
            CROSSWALK_COLUMNS,
            "docs/quality/nasa_crosswalk.csv",
            "nasa_crosswalk.csv columns must be exactly: ",
        )
        for error in errors:
            result.add_error(error)
        if not rows:
            result.add_error("nasa_crosswalk.csv must contain at least one mapping row")
            return
        for row in rows:
            control_id = (row.get("control_id") or "").strip()
            source_standard = (row.get("source_standard") or "").strip()
            repo_artifact = (row.get("repo_artifact") or "").strip()
            verification = (row.get("verification") or "").strip()
            if not control_id:
                result.add_error("nasa_crosswalk.csv row has empty control_id")
            if source_standard not in SUPPORTED_STANDARDS:
                result.add_error(f"nasa_crosswalk.csv: unsupported source_standard '{source_standard}'")
            if not verification:
                result.add_error(f"{control_id or '<missing control>'}: missing verification field")
            if not repo_artifact:
                result.add_error(f"{control_id or '<missing control>'}: missing repo_artifact")
                continue
            if not (root / repo_artifact).exists():
                result.add_error(f"{control_id or '<missing control>'}: repo_artifact does not exist: {repo_artifact}")
                continue
            if repo_artifact == "AGENTS.md" and not self._git.is_tracked(root, repo_artifact):
                result.add_error(f"{control_id or '<missing control>'}: repo_artifact must be git-tracked: {repo_artifact}")
