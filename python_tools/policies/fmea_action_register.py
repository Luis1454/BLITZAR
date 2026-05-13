# @file python_tools/policies/fmea_action_register.py
# @author Luis1454
# @project BLITZAR
# @brief Python quality and automation support for BLITZAR governance.

from __future__ import annotations

import json
from pathlib import Path

ALLOWED_STATUSES = {"open", "in-progress", "closed"}
ALLOWED_RISKS = {"Low", "Medium", "High"}


# @brief Defines the fmea action register error type contract.
# @param None This contract does not take explicit parameters.
# @note Keep construction and side effects explicit for deterministic quality gates.
class FmeaActionRegisterError(RuntimeError):
    pass


# @brief Defines the fmea action register type contract.
# @param None This contract does not take explicit parameters.
# @note Keep construction and side effects explicit for deterministic quality gates.
class FmeaActionRegister:
    # @brief Documents the load operation contract.
    # @param root Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def load(self, root: Path) -> list[dict[str, object]]:
        path = root / "docs/quality/manifest/fmea_actions.json"
        payload = json.loads(path.read_text(encoding="utf-8"))
        rows = payload.get("fmea_actions")
        if not isinstance(rows, list):
            raise FmeaActionRegisterError("fmea_actions payload must be a list")
        return [self._validate_row(row) for row in rows]

    # @brief Documents the validate row operation contract.
    # @param raw Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _validate_row(self, raw: object) -> dict[str, object]:
        if not isinstance(raw, dict):
            raise FmeaActionRegisterError("fmea_actions entries must be objects")
        row: dict[str, object] = {
            "id": self._require_string(raw, "id"),
            "owner": self._require_string(raw, "owner"),
            "status": self._require_string(raw, "status"),
            "residual_risk": self._require_string(raw, "residual_risk"),
            "linked_tasks": self._require_list(raw, "linked_tasks"),
            "verification_evidence": self._require_list(raw, "verification_evidence"),
        }
        if row["status"] not in ALLOWED_STATUSES:
            raise FmeaActionRegisterError(f"{row['id']}: status must be one of {sorted(ALLOWED_STATUSES)}")
        if row["residual_risk"] not in ALLOWED_RISKS:
            raise FmeaActionRegisterError(f"{row['id']}: residual_risk must be one of {sorted(ALLOWED_RISKS)}")
        if row["residual_risk"] in {"Medium", "High"} and not row["linked_tasks"]:
            raise FmeaActionRegisterError(f"{row['id']}: Medium/High residual risk requires at least one linked task")
        if row["status"] == "closed" and not row["verification_evidence"]:
            raise FmeaActionRegisterError(f"{row['id']}: closed mitigations require linked verification evidence")
        return row

    @staticmethod
    # @brief Documents the require string operation contract.
    # @param row Input value used by this contract.
    # @param field Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _require_string(row: dict[str, object], field: str) -> str:
        value = row.get(field)
        if not isinstance(value, str) or not value.strip():
            raise FmeaActionRegisterError(f"missing required field: {field}")
        return value.strip()

    @staticmethod
    # @brief Documents the require list operation contract.
    # @param row Input value used by this contract.
    # @param field Input value used by this contract.
    # @return Value produced by this contract when applicable.
    # @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
    def _require_list(row: dict[str, object], field: str) -> list[str]:
        value = row.get(field)
        if not isinstance(value, list):
            raise FmeaActionRegisterError(f"missing required list field: {field}")
        result: list[str] = []
        for item in value:
            if not isinstance(item, str) or not item.strip():
                raise FmeaActionRegisterError(f"{field} entries must be non-empty strings")
            result.append(item.strip())
        return result
