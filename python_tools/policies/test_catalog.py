#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path

from python_tools.core.base_check import BaseCheck
from python_tools.core.io import REQ_ID_RE, TEST_MACRO_RE, CsvLoader, JsonLoader, collect_test_ids
from python_tools.core.models import CheckContext, CheckResult

CATALOG_PATH = "docs/quality/test_catalog.csv"
CATALOG_COLUMNS = ("test_code", "test_id", "req_ids", "source")
TEST_CODE_RE = re.compile(r"^TST-[A-Z]{3}-[A-Z0-9]{2,8}-[0-9]{3}$")
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


class TestCatalogCheck(BaseCheck):
    __test__ = False
    name = "test_catalog"
    success_message = "test catalog check passed"
    failure_title = "test catalog check failed:"

    def __init__(self) -> None:
        self._csv = CsvLoader()
        self._json = JsonLoader()

    def _execute(self, context: CheckContext, result: CheckResult) -> None:
        override = context.options.get("extra_test_ids")
        extra_ids = override if isinstance(override, set) else EXTRA_TEST_IDS
        known_tests = collect_test_ids(context.root, extra_ids, TEST_MACRO_RE)
        known_req_ids = self._load_requirement_ids(context.root, result)
        rows, errors = self._csv.load_rows(
            context.root / CATALOG_PATH,
            CATALOG_COLUMNS,
            CATALOG_PATH,
            f"{CATALOG_PATH} columns must be exactly: ",
        )
        for error in errors:
            result.add_error(error)
        if not rows:
            result.add_error(f"{CATALOG_PATH} must contain at least one row")
        self._check_rows(rows, context.root, known_tests, known_req_ids, result)

    def _load_requirement_ids(self, root: Path, result: CheckResult) -> set[str]:
        payload, error = self._json.load(root / "docs/quality/requirements.json")
        if error is not None:
            result.add_error(f"failed to parse docs/quality/requirements.json: {error}")
            return set()
        if not isinstance(payload, dict):
            result.add_error("requirements.json root must be an object")
            return set()
        requirements = payload.get("requirements")
        if not isinstance(requirements, list):
            result.add_error("requirements.json: 'requirements' must be a list")
            return set()
        req_ids: set[str] = set()
        for req in requirements:
            if not isinstance(req, dict):
                continue
            req_id = str(req.get("id", "")).strip()
            if REQ_ID_RE.match(req_id):
                req_ids.add(req_id)
        return req_ids

    def _check_rows(
        self,
        rows: list[dict[str, str]],
        root: Path,
        known_tests: set[str],
        known_req_ids: set[str],
        result: CheckResult,
    ) -> None:
        used_codes: set[str] = set()
        used_tests: set[str] = set()
        for row in rows:
            test_code = (row.get("test_code") or "").strip()
            test_id = (row.get("test_id") or "").strip()
            req_ids_raw = (row.get("req_ids") or "").strip()
            source = (row.get("source") or "").strip()
            self._check_test_code(test_code, used_codes, result)
            self._check_test_id(test_code, test_id, used_tests, known_tests, result)
            self._check_req_ids(test_code, req_ids_raw, known_req_ids, result)
            self._check_source(test_code, source, root, result)
        for test_id in sorted(known_tests - used_tests):
            result.add_error(f"missing test_id in {CATALOG_PATH}: {test_id}")

    def _check_test_code(self, test_code: str, used_codes: set[str], result: CheckResult) -> None:
        if not test_code:
            result.add_error("test_catalog row has empty test_code")
            return
        if not TEST_CODE_RE.match(test_code):
            result.add_error(f"invalid test_code format: {test_code}")
            return
        if test_code in used_codes:
            result.add_error(f"duplicate test_code: {test_code}")
            return
        used_codes.add(test_code)

    def _check_test_id(
        self,
        test_code: str,
        test_id: str,
        used_tests: set[str],
        known_tests: set[str],
        result: CheckResult,
    ) -> None:
        if not test_id:
            result.add_error(f"{test_code or '<missing code>'}: empty test_id")
            return
        if test_id in used_tests:
            result.add_error(f"duplicate test_id in catalog: {test_id}")
            return
        used_tests.add(test_id)
        if test_id not in known_tests:
            result.add_error(f"{test_code or '<missing code>'}: unknown test_id '{test_id}'")

    def _check_req_ids(self, test_code: str, req_ids_raw: str, known_req_ids: set[str], result: CheckResult) -> None:
        if not req_ids_raw:
            result.add_error(f"{test_code or '<missing code>'}: req_ids must not be empty")
            return
        for req_id in [token.strip() for token in req_ids_raw.split(";") if token.strip()]:
            if not REQ_ID_RE.match(req_id):
                result.add_error(f"{test_code or '<missing code>'}: invalid req_id format '{req_id}'")
                continue
            if req_id not in known_req_ids:
                result.add_error(f"{test_code or '<missing code>'}: unknown req_id '{req_id}'")

    def _check_source(self, test_code: str, source: str, root: Path, result: CheckResult) -> None:
        if not source:
            result.add_error(f"{test_code or '<missing code>'}: source path is empty")
            return
        if not (root / source).exists():
            result.add_error(f"{test_code or '<missing code>'}: source path does not exist: {source}")
