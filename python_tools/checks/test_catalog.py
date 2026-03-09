#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path

from python_tools.checks.evidence_registry import resolve_evidence_ref
from python_tools.checks.quality_manifest import QUALITY_MANIFEST_PATH, QualityManifestLoader
from python_tools.checks.test_inventory import TestInventory
from python_tools.core.base_check import BaseCheck
from python_tools.core.io import REQ_ID_RE
from python_tools.core.models import CheckContext, CheckResult
from python_tools.core.typing_ext import JsonValue

TEST_CODE_RE = re.compile(r"^TST-[A-Z]{3}-[A-Z0-9]{2,8}-[0-9]{3}$")
TEST_GROUPS_KEY = "test_groups"


class TestCatalogCheck(BaseCheck):
    __test__ = False
    name = "test_catalog"
    success_message = "test catalog check passed"
    failure_title = "test catalog check failed:"

    def __init__(self) -> None:
        self._manifest = QualityManifestLoader()
        self._inventory = TestInventory(self._manifest)

    def _execute(self, context: CheckContext, result: CheckResult) -> None:
        manifest = self._manifest.load(context.root, result)
        known_tests = self._inventory.collect_known_test_ids(
            context.root,
            manifest,
            result,
            override=context.options.get("extra_test_ids"),
        )
        known_req_ids = self._inventory.requirement_ids(manifest, result)
        rows = self._inventory.build_catalog_rows(manifest, result, TEST_GROUPS_KEY)
        if not rows:
            result.add_error(f"{QUALITY_MANIFEST_PATH}: '{TEST_GROUPS_KEY}' must contain at least one row")
        self._check_rows(rows, context.root, known_tests, known_req_ids, result)

    def _load_catalog_rows(self, manifest: dict[str, JsonValue], result: CheckResult) -> list[dict[str, JsonValue]]:
        return self._inventory.build_catalog_rows(manifest, result, TEST_GROUPS_KEY)

    def _load_requirement_ids(self, manifest: dict[str, JsonValue], result: CheckResult) -> set[str]:
        return self._inventory.requirement_ids(manifest, result)

    def _check_rows(
        self,
        rows: list[dict[str, JsonValue]],
        root: Path,
        known_tests: set[str],
        known_req_ids: set[str],
        result: CheckResult,
    ) -> None:
        used_codes: set[str] = set()
        used_tests: set[str] = set()
        for row in rows:
            test_code = self._as_string(row.get("test_code"))
            test_id = self._as_string(row.get("test_id"))
            req_ids = self._to_req_ids(row.get("req_ids"))
            source = self._as_string(row.get("source"))
            self._check_test_code(test_code, used_codes, result)
            self._check_test_id(test_code, test_id, used_tests, known_tests, result)
            self._check_req_ids(test_code, req_ids, known_req_ids, result)
            self._check_source(test_code, source, root, result)
        for test_id in sorted(known_tests - used_tests):
            result.add_error(f"missing test_id in {QUALITY_MANIFEST_PATH}: {test_id}")

    def _check_test_code(self, test_code: str, used_codes: set[str], result: CheckResult) -> None:
        if not test_code:
            result.add_error("test catalog row has empty test_code")
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

    def _check_req_ids(self, test_code: str, req_ids: list[str], known_req_ids: set[str], result: CheckResult) -> None:
        if not req_ids:
            result.add_error(f"{test_code or '<missing code>'}: req_ids must not be empty")
            return
        for req_id in req_ids:
            if not REQ_ID_RE.match(req_id):
                result.add_error(f"{test_code or '<missing code>'}: invalid req_id format '{req_id}'")
                continue
            if req_id not in known_req_ids:
                result.add_error(f"{test_code or '<missing code>'}: unknown req_id '{req_id}'")

    def _check_source(self, test_code: str, source_ref: str, root: Path, result: CheckResult) -> None:
        if not source_ref:
            result.add_error(f"{test_code or '<missing code>'}: source ref is empty")
            return
        source_path, source_error = resolve_evidence_ref(root, source_ref)
        if source_error is not None:
            result.add_error(f"{test_code or '<missing code>'}: {source_error}")
            return
        if source_path is None:
            result.add_error(f"{test_code or '<missing code>'}: unresolved source ref: {source_ref}")
            return
        if not (root / source_path).exists():
            result.add_error(f"{test_code or '<missing code>'}: source ref does not exist: {source_ref} -> {source_path}")

    @staticmethod
    def _as_string(value: JsonValue | None) -> str:
        return value.strip() if isinstance(value, str) else ""

    @staticmethod
    def _to_req_ids(value: JsonValue | None) -> list[str]:
        if isinstance(value, list):
            return [item.strip() for item in value if isinstance(item, str) and item.strip()]
        if isinstance(value, str):
            return [token.strip() for token in value.split(";") if token.strip()]
        return []

