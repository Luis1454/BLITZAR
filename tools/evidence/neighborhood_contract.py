"""Validate the local-neighbor qualification contract and evidence records."""

from __future__ import annotations

import json
import math
import pathlib
from typing import Any


SCHEMA_VERSION = 1
RECORD_SCHEMA_VERSION = 1
SCENARIOS = {"dense", "sparse", "clustered", "moving"}
CANDIDATES = {"cell-linked-v1", "spatial-hash-v1", "hilbert-order-v1", "verlet-list-v1"}
INTEGER_FIELDS = {
    "schema",
    "seed",
    "particles",
    "steps",
    "build_ns",
    "query_ns",
    "total_ns",
    "rebuild_count",
    "neighbor_count",
    "reference_count",
    "memory_bytes",
    "candidate_hash",
    "reference_hash",
    "ordering_hash",
    "octree_build_ns",
    "octree_cells",
    "octree_memory_bytes",
    "octree_hash",
}
FLOAT_FIELDS = {"radius", "skin"}
BOOLEAN_FIELDS = {"finite", "correct", "repeatable", "selected"}
REQUIRED_FIELDS = INTEGER_FIELDS | FLOAT_FIELDS | BOOLEAN_FIELDS | {"scenario", "candidate"}


def load_contract(root: pathlib.Path) -> dict[str, Any]:
    value = json.loads((root / "plan" / "neighborhood.json").read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise ValueError("neighborhood contract must be a JSON object")
    return value


def _integer(value: Any) -> bool:
    return isinstance(value, int) and not isinstance(value, bool)


def validate_contract(contract: dict[str, Any]) -> list[str]:
    errors: list[str] = []
    if contract.get("schema_version") != SCHEMA_VERSION:
        errors.append(f"schema_version must be {SCHEMA_VERSION}")
    if contract.get("record_schema_version") != RECORD_SCHEMA_VERSION:
        errors.append(f"record_schema_version must be {RECORD_SCHEMA_VERSION}")
    if contract.get("target") != "blitzar_neighborhood_test":
        errors.append("target must be blitzar_neighborhood_test")
    if contract.get("runner") != "tools/evidence/neighborhood_evidence.py":
        errors.append("runner must be tools/evidence/neighborhood_evidence.py")
    if contract.get("precision") != "float64":
        errors.append("precision must remain float64")
    if not isinstance(contract.get("plan_version"), str):
        errors.append("plan_version must be a string")
    if not _integer(contract.get("seed")) or contract["seed"] < 0:
        errors.append("seed must be a non-negative integer")
    if contract.get("particle_count") != 96 or contract.get("steps") != 6:
        errors.append("particle_count and steps must remain 96 and 6")
    errors.extend(_validate_interaction(contract.get("interaction")))
    errors.extend(_validate_scenarios(contract.get("scenarios")))
    errors.extend(_validate_candidates(contract.get("candidates")))
    if contract.get("selection") != {scenario: "cell-linked-v1" for scenario in SCENARIOS}:
        errors.append("selection must choose cell-linked-v1 for every scenario")
    if contract.get("rebuild_schedule") != {
        "static_steps": 1,
        "moving_steps": 6,
        "moving_velocity_exceeds_threshold": True,
    }:
        errors.append("rebuild schedule does not match the deterministic workload")
    if contract.get("memory_bound_bytes") != 2000000:
        errors.append("memory_bound_bytes must remain 2000000")
    if set(contract.get("required_record_fields", [])) != REQUIRED_FIELDS:
        errors.append("required_record_fields must declare the complete neighbor record")
    baseline = contract.get("tree_baseline")
    if not isinstance(baseline, dict) or baseline.get("local_neighbor_oracle") is not False:
        errors.append("Octree baseline must remain explicitly separate from local neighbors")
    artifacts = contract.get("artifacts")
    if not isinstance(artifacts, dict) or artifacts.get("generated_outside_source") is not True:
        errors.append("artifacts must require external generated evidence")
    return errors


def _validate_interaction(value: Any) -> list[str]:
    if not isinstance(value, dict):
        return ["interaction must be an object"]
    errors: list[str] = []
    if value.get("radius") != 0.75 or value.get("skin") != 0.4:
        errors.append("interaction radius and skin must remain 0.75 and 0.4")
    if value.get("boundary", "").find("finite non-periodic") < 0:
        errors.append("interaction boundary must be finite and non-periodic")
    if value.get("neighbor_order") != "target index ascending, source index ascending":
        errors.append("neighbor order must be deterministic original-index order")
    return errors


def _validate_scenarios(value: Any) -> list[str]:
    if not isinstance(value, list) or {item.get("id") for item in value} != SCENARIOS:
        return ["scenarios must contain dense, sparse, clustered, and moving"]
    motions = {item["id"]: item.get("motion") for item in value}
    if motions != {"dense": "static", "sparse": "static", "clustered": "static", "moving": "moving"}:
        return ["scenario motion policies are invalid"]
    return []


def _validate_candidates(value: Any) -> list[str]:
    if not isinstance(value, list) or {item.get("id") for item in value} != CANDIDATES:
        return ["candidates must contain the four declared local structures"]
    selected = [item.get("id") for item in value if item.get("selected") is True]
    return [] if selected == ["cell-linked-v1"] else ["candidate selection must have one cell-linked entry"]


def _parse_tokens(line: str) -> dict[str, Any]:
    prefix = "BLITZAR NEIGHBOR "
    if not line.startswith(prefix):
        raise ValueError("unexpected neighbor record prefix")
    record: dict[str, Any] = {}
    for token in line[len(prefix) :].split():
        key, separator, value = token.partition("=")
        if not separator or key in record:
            raise ValueError("malformed neighbor record")
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
    return _parse_tokens(line) if line.startswith("BLITZAR NEIGHBOR ") else None


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
        return [f"neighbor record is missing fields: {missing}"]
    errors = _validate_values(record)
    if record["schema"] != contract["record_schema_version"]:
        errors.append("neighbor record schema does not match the contract")
    if record["seed"] != contract["seed"] or record["particles"] != contract["particle_count"]:
        errors.append("neighbor record identity does not match the contract")
    if record["steps"] != contract["steps"] or record["radius"] != 0.75 or record["skin"] != 0.4:
        errors.append("neighbor record parameters do not match the contract")
    if record["scenario"] not in SCENARIOS or record["candidate"] not in CANDIDATES:
        errors.append("neighbor scenario or candidate is not declared")
    if record["build_ns"] <= 0 or record["query_ns"] <= 0 or record["total_ns"] <= 0:
        errors.append("neighbor timing must be positive")
    if record["total_ns"] != record["build_ns"] + record["query_ns"]:
        errors.append("neighbor total timing must equal build plus query timing")
    return errors


def validate_records(records: list[dict[str, Any]], contract: dict[str, Any]) -> list[str]:
    errors = validate_contract(contract)
    if errors:
        return errors
    if len(records) != len(SCENARIOS) * len(CANDIDATES):
        errors.append(f"neighbor matrix mismatch: expected 16, got {len(records)}")
    seen: set[tuple[str, str]] = set()
    for record in records:
        errors.extend(validate_record(record, contract))
        identifier = (record.get("scenario"), record.get("candidate"))
        if identifier in seen:
            errors.append(f"duplicate neighbor record: {identifier}")
        seen.add(identifier)
        if record.get("memory_bytes", 0) > contract["memory_bound_bytes"]:
            errors.append(f"neighbor memory bound exceeded: {identifier}")
        if record.get("neighbor_count") != record.get("reference_count"):
            errors.append(f"neighbor count mismatch: {identifier}")
        if record.get("candidate_hash") != record.get("reference_hash"):
            errors.append(f"neighbor hash mismatch: {identifier}")
        if not all(record.get(field) for field in ("finite", "correct", "repeatable")):
            errors.append(f"neighbor qualification flags failed: {identifier}")
        expected = contract["rebuild_schedule"]["moving_steps"] if record.get("scenario") == "moving" else 1
        if record.get("rebuild_count") != expected:
            errors.append(f"neighbor rebuild schedule mismatch: {identifier}")
    for scenario in SCENARIOS:
        group = [item for item in records if item.get("scenario") == scenario]
        selected = [item for item in group if item.get("selected")]
        if len(selected) != 1 or selected[0].get("candidate") != contract["selection"][scenario]:
            errors.append(f"neighbor selection mismatch: {scenario}")
        tree_values = {
            (item.get("octree_build_ns"), item.get("octree_cells"), item.get("octree_hash"))
            for item in group
        }
        if len(tree_values) != 1 or any(item.get("octree_memory_bytes", 0) <= 0 for item in group):
            errors.append(f"Octree baseline mismatch: {scenario}")
    return errors
