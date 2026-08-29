"""Validate the bounded block-time qualification contract and records."""

from __future__ import annotations

import json
import math
import pathlib
from typing import Any


SCHEMA_VERSION = 1
RECORD_SCHEMA_VERSION = 1
WORKLOADS = {"heterogeneous-v1", "clustered-v1", "migration-v1"}
INTEGER_FIELDS = {
    "schema",
    "seed",
    "particles",
    "horizon_ticks",
    "sync_interval",
    "migration_tick",
    "fixed_events",
    "block_events",
    "fixed_elapsed_ns",
    "block_elapsed_ns",
    "fixed_event_hash",
    "block_event_hash",
    "restart_event_hash",
    "rollback_event_hash",
    "fixed_ownership_hash",
    "block_ownership_hash",
    "initial_input_hash",
    "final_input_hash",
}
FLOAT_FIELDS = {"modeled_speedup"}
BOOLEAN_FIELDS = {
    "active_ordered",
    "deterministic",
    "ledger_conserved",
    "migration",
    "restart_compatible",
    "rollback_transactional",
    "state_unchanged",
    "candidate_selected",
}
STRING_FIELDS = {
    "workload",
    "reference",
    "candidate",
    "speedup_scope",
    "decision",
}
REQUIRED_FIELDS = INTEGER_FIELDS | FLOAT_FIELDS | BOOLEAN_FIELDS | STRING_FIELDS


def load_contract(root: pathlib.Path) -> dict[str, Any]:
    value = json.loads((root / "plan" / "block_time.json").read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise ValueError("block-time contract must be a JSON object")
    return value


def _integer(value: Any) -> bool:
    return isinstance(value, int) and not isinstance(value, bool)


def _expected_semantics() -> dict[str, str]:
    return {
        "time_bin": "bin b executes on base tick t when t modulo 2^b is zero",
        "active_order": "ascending stable particle index",
        "ownership": "one owner per particle; owner changes only at synchronization ticks",
        "synchronization": "global schedule boundary every eight base ticks",
        "migration": "ownership migration is applied at tick 32 before the next active work list",
        "rollback": "restore tick, ownership, counters, and event hash before retry",
        "restart": "resume from a captured schedule state without replaying earlier events",
    }


def validate_contract(contract: dict[str, Any]) -> list[str]:
    errors: list[str] = []
    if contract.get("schema_version") != SCHEMA_VERSION:
        errors.append(f"schema_version must be {SCHEMA_VERSION}")
    if contract.get("record_schema_version") != RECORD_SCHEMA_VERSION:
        errors.append(f"record_schema_version must be {RECORD_SCHEMA_VERSION}")
    if contract.get("target") != "blitzar_block_time_test":
        errors.append("target must be blitzar_block_time_test")
    if contract.get("runner") != "tools/evidence/block_time_evidence.py":
        errors.append("runner must be tools/evidence/block_time_evidence.py")
    if contract.get("precision") != "float64":
        errors.append("precision must remain float64")
    if not isinstance(contract.get("plan_version"), str):
        errors.append("plan_version must be a string")
    if not _integer(contract.get("seed")) or contract["seed"] < 0:
        errors.append("seed must be a non-negative integer")
    if contract.get("horizon_ticks") != 64 or contract.get("synchronization_interval") != 8:
        errors.append("horizon and synchronization interval must remain 64 and 8")
    if contract.get("migration_tick") != 32 or contract.get("max_time_bin") != 3:
        errors.append("migration tick and maximum time bin must remain 32 and 3")
    if contract.get("semantics") != _expected_semantics():
        errors.append("schedule semantics are not frozen")
    errors.extend(_validate_identity(contract.get("reference"), "fixed-kdk-v1", "reference"))
    errors.extend(
        _validate_identity(contract.get("candidate"), "block-kdk-schedule-v1", "candidate")
    )
    workloads = contract.get("workloads")
    if not isinstance(workloads, list) or {item.get("id") for item in workloads} != WORKLOADS:
        errors.append("workloads must contain the three declared block-time cases")
    decision = contract.get("decision")
    if not isinstance(decision, dict) or decision.get("status") != "not-selected":
        errors.append("block-time candidate must remain not-selected")
    else:
        if decision.get("production_integrator") != "fixed-kdk-v1":
            errors.append("fixed KDK must remain the production integrator")
        if len(decision.get("promotion_requires", [])) != 4:
            errors.append("block-time promotion requirements are incomplete")
    proxy = contract.get("proxy_policy")
    if not isinstance(proxy, dict) or "scheduler loop wall time only" not in proxy.get("timing", ""):
        errors.append("block-time timing must remain a scheduler-only proxy")
    if not isinstance(proxy, dict) or "particle ledger" not in proxy.get("conservation", ""):
        errors.append("block-time conservation must remain a ledger-only proxy")
    if set(contract.get("required_record_fields", [])) != REQUIRED_FIELDS:
        errors.append("required_record_fields must declare the complete block-time record")
    artifacts = contract.get("artifacts")
    if not isinstance(artifacts, dict) or artifacts.get("generated_outside_source") is not True:
        errors.append("artifacts must require external generated evidence")
    return errors


def _validate_identity(value: Any, identifier: str, name: str) -> list[str]:
    if not isinstance(value, dict) or value.get("id") != identifier:
        return [f"{name} must identify {identifier}"]
    return []


def _parse_tokens(line: str) -> dict[str, Any]:
    prefix = "BLITZAR BLOCK "
    if not line.startswith(prefix):
        raise ValueError("unexpected block-time record prefix")
    record: dict[str, Any] = {}
    for token in line[len(prefix) :].split():
        key, separator, value = token.partition("=")
        if not separator or not key or not value or key in record:
            raise ValueError("malformed block-time record")
        if key in INTEGER_FIELDS:
            record[key] = int(value)
        elif key in FLOAT_FIELDS:
            record[key] = float(value)
        elif key in BOOLEAN_FIELDS:
            if value not in {"0", "1"}:
                raise ValueError(f"invalid boolean field: {key}")
            record[key] = value == "1"
        else:
            record[key] = value
    return record


def parse_record(line: str) -> dict[str, Any] | None:
    return _parse_tokens(line) if line.startswith("BLITZAR BLOCK ") else None


def _validate_values(record: dict[str, Any]) -> list[str]:
    errors: list[str] = []
    for field in INTEGER_FIELDS:
        if not _integer(record[field]) or record[field] < 0:
            errors.append(f"integer field is invalid: {field}")
    for field in FLOAT_FIELDS:
        if not isinstance(record[field], (int, float)) or not math.isfinite(record[field]):
            errors.append(f"float field is invalid: {field}")
    for field in BOOLEAN_FIELDS:
        if not isinstance(record[field], bool):
            errors.append(f"boolean field is invalid: {field}")
    return errors


def validate_record(record: dict[str, Any], contract: dict[str, Any]) -> list[str]:
    missing = sorted(REQUIRED_FIELDS - set(record))
    if missing:
        return [f"block-time record is missing fields: {missing}"]
    errors = _validate_values(record)
    if record["schema"] != contract["record_schema_version"]:
        errors.append("block-time record schema does not match the contract")
    if record["seed"] != contract["seed"] or record["particles"] != 32:
        errors.append("block-time record identity does not match the contract")
    if (
        record["horizon_ticks"] != contract["horizon_ticks"]
        or record["sync_interval"] != contract["synchronization_interval"]
        or record["migration_tick"] != contract["migration_tick"]
    ):
        errors.append("block-time record schedule parameters do not match the contract")
    if record["workload"] not in WORKLOADS:
        errors.append("block-time workload is not declared")
    if record["reference"] != contract["reference"]["id"]:
        errors.append("block-time reference is invalid")
    if record["candidate"] != contract["candidate"]["id"]:
        errors.append("block-time candidate is invalid")
    if record["speedup_scope"] != "schedule-work-proxy" or record["decision"] != "not-selected":
        errors.append("block-time result scope or decision is invalid")
    if record["fixed_elapsed_ns"] <= 0 or record["block_elapsed_ns"] <= 0:
        errors.append("block-time timings must be positive")
    if record["fixed_events"] != record["particles"] * record["horizon_ticks"]:
        errors.append("fixed event ledger does not cover the full horizon")
    if record["block_events"] <= 0 or record["block_events"] >= record["fixed_events"]:
        errors.append("block schedule must reduce positive event work")
    expected_speedup = record["fixed_events"] / record["block_events"]
    if not math.isclose(record["modeled_speedup"], expected_speedup, rel_tol=1e-12):
        errors.append("modeled speedup is inconsistent with event counts")
    if record["fixed_event_hash"] == record["block_event_hash"]:
        errors.append("fixed and block event hashes must differ")
    if record["restart_event_hash"] != record["block_event_hash"]:
        errors.append("restart hash does not reproduce block schedule")
    if record["rollback_event_hash"] != record["block_event_hash"]:
        errors.append("rollback hash does not reproduce block schedule")
    if record["fixed_ownership_hash"] != record["block_ownership_hash"]:
        errors.append("fixed and block ownership hashes differ")
    if record["initial_input_hash"] != record["final_input_hash"]:
        errors.append("qualification mutated its input workload")
    if record["migration_tick"] % record["sync_interval"] != 0:
        errors.append("migration must occur on a synchronization boundary")
    if not all(record[field] for field in BOOLEAN_FIELDS if field != "candidate_selected"):
        errors.append("block-time qualification flags are incomplete")
    if record["candidate_selected"]:
        errors.append("block-time candidate must not be selected")
    return errors


def validate_records(records: list[dict[str, Any]], contract: dict[str, Any]) -> list[str]:
    errors = validate_contract(contract)
    if errors:
        return errors
    if len(records) != len(WORKLOADS):
        errors.append(f"block-time matrix mismatch: expected 3, got {len(records)}")
    seen: set[str] = set()
    for record in records:
        errors.extend(validate_record(record, contract))
        workload = record.get("workload")
        if workload in seen:
            errors.append(f"duplicate block-time record: {workload}")
        seen.add(workload)
    if seen != WORKLOADS:
        errors.append("block-time matrix does not cover every workload")
    return errors
