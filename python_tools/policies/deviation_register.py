#!/usr/bin/env python3
# @file python_tools/policies/deviation_register.py
# @author Luis1454
# @project BLITZAR
# @brief Python quality and automation support for BLITZAR governance.

from __future__ import annotations

import re
from datetime import date
from pathlib import Path

from python_tools.core.models import CheckResult, JsonValue
from python_tools.policies.quality_manifest import (
    QUALITY_MANIFEST_PATH,
    REQUIREMENTS_KEY,
    QualityManifestLoader,
    resolve_evidence_ref,
)

DEVIATIONS_KEY = "deviations"
DEVIATION_ID_RE = re.compile(r"^(DEV|WVR)-[A-Z0-9]+-\d{3}$")
ALLOWED_KINDS = {"deviation", "waiver"}
ALLOWED_STATUSES = {"open", "closed"}


# @brief Defines the deviation register type contract.
# @param None This contract does not take explicit parameters.
# @note Keep construction and side effects explicit for deterministic quality gates.
class DeviationRegister:
    # @brief Documents the init operation contract.
    # @param None This contract does not take explicit parameters.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def __init__(self) -> None:
        self._manifest = QualityManifestLoader()

    # @brief Documents the load operation contract.
    # @param root Input value used by this contract.
    # @param manifest Input value used by this contract.
    # @param requirement_ids Input value used by this contract.
    # @param result Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def load(
        self,
        root: Path,
        manifest: dict[str, JsonValue],
        requirement_ids: set[str],
        result: CheckResult,
    ) -> list[dict[str, object]]:
        rows = self._manifest.get_list(manifest, DEVIATIONS_KEY, result, keyed_field="id")
        if not rows:
            result.add_error(f"{QUALITY_MANIFEST_PATH}: '{DEVIATIONS_KEY}' must contain at least one row")
            return []
        parsed: list[dict[str, object]] = []
        for row in rows:
            parsed_row = self._parse_row(root, row, requirement_ids, result)
            if parsed_row:
                parsed.append(parsed_row)
        return parsed

    # @brief Documents the load open with errors operation contract.
    # @param root Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def load_open_with_errors(self, root: Path) -> tuple[list[dict[str, object]], list[str]]:
        payload, errors = self._manifest.load_with_errors(root)
        if errors:
            return [], errors
        requirements = payload.get(REQUIREMENTS_KEY)
        requirement_ids = set(requirements.keys()) if isinstance(requirements, dict) else set()
        result = CheckResult(name="deviations")
        rows = self.load(root, payload, requirement_ids, result)
        return [row for row in rows if row["status"] == "open"], result.errors

    # @brief Documents the parse row operation contract.
    # @param root Input value used by this contract.
    # @param row Input value used by this contract.
    # @param requirement_ids Input value used by this contract.
    # @param result Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _parse_row(
        self,
        root: Path,
        row: dict[str, JsonValue],
        requirement_ids: set[str],
        result: CheckResult,
    ) -> dict[str, object] | None:
        deviation_id = self._as_string(row.get("id"))
        if not DEVIATION_ID_RE.match(deviation_id):
            result.add_error(f"invalid deviation id format: {deviation_id}")
            return None
        kind = self._required_string(row, deviation_id, "kind", result)
        status = self._required_string(row, deviation_id, "status", result)
        scope = self._required_string(row, deviation_id, "scope", result)
        owner = self._required_string(row, deviation_id, "owner", result)
        approver = self._required_string(row, deviation_id, "approver", result)
        introduced_on = self._required_string(row, deviation_id, "introduced_on", result)
        review_by = self._required_string(row, deviation_id, "review_by", result)
        rationale = self._required_string(row, deviation_id, "rationale", result)
        mitigation = self._required_string(row, deviation_id, "mitigation", result)
        closure_criteria = self._required_string(row, deviation_id, "closure_criteria", result)
        paths = self._string_list(row.get("paths"))
        requirements = self._string_list(row.get("requirements"))
        artifacts = self._string_list(row.get("artifacts"))
        if kind not in ALLOWED_KINDS:
            result.add_error(f"{deviation_id}: unsupported kind '{kind}'")
        if status not in ALLOWED_STATUSES:
            result.add_error(f"{deviation_id}: unsupported status '{status}'")
        self._validate_date(introduced_on, deviation_id, "introduced_on", result)
        self._validate_date(review_by, deviation_id, "review_by", result)
        if not paths:
            result.add_error(f"{deviation_id}: missing paths list")
        if not requirements and not artifacts:
            result.add_error(f"{deviation_id}: add at least one linked requirement or artifact")
        for requirement_id in requirements:
            if requirement_id not in requirement_ids:
                result.add_error(f"{deviation_id}: unknown linked requirement id: {requirement_id}")
        for artifact_ref in artifacts:
            artifact_path, artifact_error = resolve_evidence_ref(root, artifact_ref)
            if artifact_error is not None:
                result.add_error(f"{deviation_id}: {artifact_error}")
                continue
            if artifact_path is None or not (root / artifact_path).exists():
                result.add_error(f"{deviation_id}: referenced artifact does not exist: {artifact_ref}")
        return {
            "id": deviation_id,
            "kind": kind,
            "status": status,
            "scope": scope,
            "owner": owner,
            "approver": approver,
            "introduced_on": introduced_on,
            "review_by": review_by,
            "rationale": rationale,
            "mitigation": mitigation,
            "closure_criteria": closure_criteria,
            "paths": paths,
            "requirements": requirements,
            "artifacts": artifacts,
        }

    @staticmethod
    # @brief Documents the required string operation contract.
    # @param row Input value used by this contract.
    # @param deviation_id Input value used by this contract.
    # @param field Input value used by this contract.
    # @param result Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _required_string(row: dict[str, JsonValue], deviation_id: str, field: str, result: CheckResult) -> str:
        value = DeviationRegister._as_string(row.get(field))
        if not value:
            result.add_error(f"{deviation_id}: missing {field}")
        return value

    @staticmethod
    # @brief Documents the validate date operation contract.
    # @param value Input value used by this contract.
    # @param deviation_id Input value used by this contract.
    # @param field Input value used by this contract.
    # @param result Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _validate_date(value: str, deviation_id: str, field: str, result: CheckResult) -> None:
        try:
            date.fromisoformat(value)
        except ValueError:
            result.add_error(f"{deviation_id}: invalid ISO date in {field}: {value}")

    @staticmethod
    # @brief Documents the as string operation contract.
    # @param value Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _as_string(value: JsonValue | None) -> str:
        return value.strip() if isinstance(value, str) else ""

    @staticmethod
    # @brief Documents the string list operation contract.
    # @param value Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _string_list(value: JsonValue | None) -> list[str]:
        if not isinstance(value, list):
            return []
        return [item.strip() for item in value if isinstance(item, str) and item.strip()]
