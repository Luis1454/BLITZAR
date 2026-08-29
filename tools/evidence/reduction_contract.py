"""Validate the deterministic compensated-reduction evidence contract."""

from __future__ import annotations

import json
import math
import pathlib
from typing import Any


SCHEMA_VERSION = 1
RECORD_SCHEMA_VERSION = 1
INTEGER_FIELDS = {
    "schema",
    "seed",
    "terms",
    "elapsed_ns",
    "input_hash",
    "value_hash",
    "steps",
    "state_hash",
}
FLOAT_FIELDS = {
    "terms_per_second",
    "expected",
    "value",
    "absolute_error",
    "relative_error",
    "max_relative_energy_error",
    "final_energy",
    "final_momentum_norm",
}
BOOLEAN_FIELDS = {
    "repeatable",
    "finite",
    "vectorization_eligible",
    "default_policy_match",
    "selected",
}
POLICIES = {"plain-v1", "kahan-v1", "neumaier-v1"}
WORKLOADS = {"force", "kinetic-energy", "potential-energy", "momentum"}


def load_contract(root: pathlib.Path) -> dict[str, Any]:
    value = json.loads((root / "plan" / "reduction.json").read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise ValueError("reduction contract must be a JSON object")
    return value


def _is_integer(value: Any) -> bool:
    return isinstance(value, int) and not isinstance(value, bool)


def validate_contract(contract: dict[str, Any]) -> list[str]:
    errors: list[str] = []
    if contract.get("schema_version") != SCHEMA_VERSION:
        errors.append(f"schema_version must be {SCHEMA_VERSION}")
    if contract.get("record_schema_version") != RECORD_SCHEMA_VERSION:
        errors.append(f"record_schema_version must be {RECORD_SCHEMA_VERSION}")
    if contract.get("target") != "blitzar_reduction_test":
        errors.append("target must be blitzar_reduction_test")
    if contract.get("runner") != "tools/evidence/reduction_evidence.py":
        errors.append("runner must be tools/evidence/reduction_evidence.py")
    if not isinstance(contract.get("plan_version"), str):
        errors.append("plan_version must be a string")
    if not _is_integer(contract.get("seed")) or contract["seed"] < 0:
        errors.append("seed must be a non-negative integer")
    if contract.get("term_counts") != [65536]:
        errors.append("term_counts must remain [65536]")
    if contract.get("long_run_steps") != 4096:
        errors.append("long_run_steps must remain 4096")
    errors.extend(_validate_policies(contract.get("policies")))
    errors.extend(_validate_workloads(contract.get("workloads")))
    selection = contract.get("selection")
    expected_selection = {
        "force": "plain-v1",
        "kinetic-energy": "neumaier-v1",
        "potential-energy": "neumaier-v1",
        "momentum": "neumaier-v1",
        "long-run": "neumaier-v1",
    }
    if selection != expected_selection:
        errors.append("selection does not match the accepted policy")
    required = contract.get("required_record_fields")
    expected = INTEGER_FIELDS | FLOAT_FIELDS | BOOLEAN_FIELDS | {"workload", "policy"}
    expected -= {"steps", "state_hash", "max_relative_energy_error", "final_energy", "final_momentum_norm", "default_policy_match"}
    if not isinstance(required, list) or set(required) != expected:
        errors.append("required_record_fields must declare the complete reduction record")
    long_required = contract.get("required_long_run_fields")
    expected_long = INTEGER_FIELDS | FLOAT_FIELDS | BOOLEAN_FIELDS | {"policy"}
    expected_long -= {
        "terms",
        "elapsed_ns",
        "input_hash",
        "value_hash",
        "terms_per_second",
        "expected",
        "value",
        "absolute_error",
        "relative_error",
        "repeatable",
        "vectorization_eligible",
    }
    if not isinstance(long_required, list) or set(long_required) != expected_long:
        errors.append("required_long_run_fields must declare the complete long-run record")
    limits = contract.get("limits")
    if not isinstance(limits, dict) or limits.get("max_relative_energy_error") != 1.0e-8:
        errors.append("limits must preserve the long-run energy threshold")
    artifacts = contract.get("artifacts")
    if not isinstance(artifacts, dict) or artifacts.get("generated_outside_source") is not True:
        errors.append("artifacts must require external generated evidence")
    return errors


def _validate_policies(value: Any) -> list[str]:
    if not isinstance(value, list) or len(value) != 3:
        return ["policies must contain Plain, Kahan, and Neumaier"]
    identifiers = {item.get("id") for item in value if isinstance(item, dict)}
    errors: list[str] = []
    if identifiers != POLICIES:
        errors.append("policies do not match the accepted candidates")
    for item in value:
        if not isinstance(item, dict) or not isinstance(item.get("id"), str):
            errors.append("every policy needs an id")
            continue
        if item.get("vectorization") not in {"eligible", "serial"}:
            errors.append(f"policy {item['id']} has invalid vectorization metadata")
    return errors


def _validate_workloads(value: Any) -> list[str]:
    if not isinstance(value, list) or len(value) != 4:
        return ["workloads must contain four cancellation cases"]
    identifiers = {item.get("id") for item in value if isinstance(item, dict)}
    return [] if identifiers == WORKLOADS else ["workloads do not match the accepted cases"]


def _parse_tokens(line: str, prefix: str) -> dict[str, Any]:
    if not line.startswith(prefix):
        raise ValueError("unexpected reduction record prefix")
    record: dict[str, Any] = {}
    for token in line[len(prefix) :].split():
        key, separator, value = token.partition("=")
        if not separator or key in record:
            raise ValueError("malformed reduction record")
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


def parse_record(line: str) -> tuple[str, dict[str, Any]] | None:
    if line.startswith("BLITZAR REDUCTION_LONG "):
        return "long", _parse_tokens(line, "BLITZAR REDUCTION_LONG ")
    if line.startswith("BLITZAR REDUCTION "):
        return "reduction", _parse_tokens(line, "BLITZAR REDUCTION ")
    return None


def _validate_values(record: dict[str, Any], fields: set[str]) -> list[str]:
    errors: list[str] = []
    for field in fields & INTEGER_FIELDS:
        if not _is_integer(record[field]) or record[field] < 0:
            errors.append(f"integer field is invalid: {field}")
    for field in fields & FLOAT_FIELDS:
        if not isinstance(record[field], (int, float)) or not math.isfinite(record[field]):
            errors.append(f"float field is invalid: {field}")
    for field in fields & BOOLEAN_FIELDS:
        if not isinstance(record[field], bool):
            errors.append(f"boolean field is invalid: {field}")
    return errors


def validate_record(record: dict[str, Any], contract: dict[str, Any]) -> list[str]:
    required = set(contract["required_record_fields"])
    missing = sorted(required - set(record))
    if missing:
        return [f"reduction record is missing fields: {missing}"]
    errors = _validate_values(record, required)
    if record["schema"] != contract["record_schema_version"]:
        errors.append("reduction record schema does not match the contract")
    if record["seed"] != contract["seed"]:
        errors.append("reduction record seed does not match the contract")
    if record["terms"] not in contract["term_counts"]:
        errors.append("reduction term count is not declared")
    if record["workload"] not in WORKLOADS or record["policy"] not in POLICIES:
        errors.append("reduction workload or policy is not declared")
    if record["elapsed_ns"] <= 0 or record["terms_per_second"] <= 0:
        errors.append("reduction timing and throughput must be positive")
    if record["absolute_error"] < 0 or record["relative_error"] < 0:
        errors.append("reduction errors must be non-negative")
    return errors


def validate_long_record(record: dict[str, Any], contract: dict[str, Any]) -> list[str]:
    required = set(contract["required_long_run_fields"])
    missing = sorted(required - set(record))
    if missing:
        return [f"long-run record is missing fields: {missing}"]
    errors = _validate_values(record, required)
    if record["schema"] != contract["record_schema_version"]:
        errors.append("long-run record schema does not match the contract")
    if record["seed"] != contract["seed"] or record["steps"] != contract["long_run_steps"]:
        errors.append("long-run identity does not match the contract")
    if record["policy"] not in POLICIES:
        errors.append("long-run policy is not declared")
    if record["max_relative_energy_error"] > contract["limits"]["max_relative_energy_error"]:
        errors.append("long-run energy error exceeds the contract")
    return errors


def validate_records(
    records: list[dict[str, Any]], long_runs: list[dict[str, Any]], contract: dict[str, Any]
) -> list[str]:
    errors = validate_contract(contract)
    if errors:
        return errors
    expected_count = len(contract["term_counts"]) * len(WORKLOADS) * len(POLICIES)
    if len(records) != expected_count:
        errors.append(f"reduction matrix mismatch: expected {expected_count}, got {len(records)}")
    seen: set[tuple[Any, ...]] = set()
    for record in records:
        errors.extend(validate_record(record, contract))
        identifier = (record.get("terms"), record.get("workload"), record.get("policy"))
        if identifier in seen:
            errors.append(f"duplicate reduction record: {identifier}")
        seen.add(identifier)
    for workload in WORKLOADS:
        group = [record for record in records if record.get("workload") == workload]
        if len({record.get("input_hash") for record in group}) != 1:
            errors.append(f"input ordering hash differs for {workload}")
        selected = [record for record in group if record.get("selected")]
        if len(selected) != 1 or selected[0].get("policy") != contract["selection"][workload]:
            errors.append(f"selected policy mismatch for {workload}")
        by_policy = {record.get("policy"): record for record in group}
        if set(by_policy) == POLICIES:
            selected_record = by_policy[contract["selection"][workload]]
            plain_record = by_policy["plain-v1"]
            if selected_record["relative_error"] > plain_record["relative_error"]:
                errors.append(f"selected reduction is less accurate than plain for {workload}")
    if len(long_runs) != len(POLICIES):
        errors.append(f"long-run matrix mismatch: expected {len(POLICIES)}, got {len(long_runs)}")
    long_seen: set[str] = set()
    for record in long_runs:
        errors.extend(validate_long_record(record, contract))
        policy = record.get("policy")
        if policy in long_seen:
            errors.append(f"duplicate long-run policy: {policy}")
        long_seen.add(policy)
        if not record.get("default_policy_match"):
            errors.append(f"default policy mismatch for {policy}")
    if len({record.get("state_hash") for record in long_runs}) != 1:
        errors.append("long-run state hash differs between reduction policies")
    selected_long = [record for record in long_runs if record.get("selected")]
    if len(selected_long) != 1 or selected_long[0].get("policy") != contract["selection"]["long-run"]:
        errors.append("selected long-run policy mismatch")
    return errors
