"""Validate the bounded BVH irregular-query qualification contract and records."""

from __future__ import annotations

import json
import math
import pathlib
from typing import Any


SCHEMA_VERSION = 1
RECORD_SCHEMA_VERSION = 1
PLAN_VERSION = "1.0.52"
WORKLOADS = {"dense", "sparse", "clustered", "moving"}
INTEGER_FIELDS = {
    "schema",
    "seed",
    "particles",
    "steps",
    "leaf_size",
    "bvh_build_ns",
    "bvh_refit_ns",
    "bvh_query_ns",
    "bvh_rebuild_ns",
    "cell_linked_build_ns",
    "cell_linked_query_ns",
    "bvh_rebuild_count",
    "bvh_refit_count",
    "bvh_rebuild_baseline_count",
    "bvh_neighbor_count",
    "cell_linked_neighbor_count",
    "reference_count",
    "bvh_memory_bytes",
    "bvh_workspace_bytes",
    "cell_linked_memory_bytes",
    "bvh_hash",
    "bvh_topology_hash",
    "cell_linked_hash",
    "reference_hash",
    "bvh_ordering_hash",
    "cell_linked_ordering_hash",
    "octree_build_ns",
    "octree_cells",
    "octree_memory_bytes",
    "octree_hash",
}
FLOAT_FIELDS = {"radius", "skin"}
BOOLEAN_FIELDS = {
    "finite",
    "correct",
    "repeatable",
    "deterministic",
    "refit_correct",
    "rebuild_correct",
    "refit_parity",
    "selected",
}
STRING_FIELDS = {"scenario", "decision"}
REQUIRED_FIELDS = INTEGER_FIELDS | FLOAT_FIELDS | BOOLEAN_FIELDS | STRING_FIELDS


def load_contract(root: pathlib.Path) -> dict[str, Any]:
    value = json.loads((root / "plan" / "bvh.json").read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise ValueError("BVH contract must be a JSON object")
    return value


def _integer(value: Any) -> bool:
    return isinstance(value, int) and not isinstance(value, bool)


def validate_contract(contract: dict[str, Any]) -> list[str]:
    errors: list[str] = []
    if contract.get("schema_version") != SCHEMA_VERSION:
        errors.append(f"schema_version must be {SCHEMA_VERSION}")
    if contract.get("record_schema_version") != RECORD_SCHEMA_VERSION:
        errors.append(f"record_schema_version must be {RECORD_SCHEMA_VERSION}")
    if contract.get("plan_version") != PLAN_VERSION:
        errors.append(f"plan_version must be {PLAN_VERSION}")
    if contract.get("target") != "blitzar_bvh_test":
        errors.append("target must be blitzar_bvh_test")
    if contract.get("runner") != "tools/evidence/bvh_evidence.py":
        errors.append("runner must be tools/evidence/bvh_evidence.py")
    if contract.get("precision") != "float64":
        errors.append("precision must remain float64")
    if contract.get("seed") != 424242 or contract.get("particle_count") != 96:
        errors.append("seed and particle count must remain 424242 and 96")
    if contract.get("steps") != 6 or contract.get("radius") != 0.75 or contract.get("skin") != 0.4:
        errors.append("steps, radius, and skin must remain 6, 0.75, and 0.4")
    if contract.get("leaf_size") != 4:
        errors.append("leaf_size must remain 4")
    if set(contract.get("workloads", [])) != WORKLOADS:
        errors.append("workloads must contain dense, sparse, clustered, and moving")
    reference = contract.get("reference")
    if not isinstance(reference, dict) or reference.get("id") != "exact-directed-neighbor-v1":
        errors.append("reference must identify exact-directed-neighbor-v1")
    selected = contract.get("selected_neighbor")
    if not isinstance(selected, dict) or selected.get("id") != "cell-linked-v1":
        errors.append("selected_neighbor must identify cell-linked-v1")
    baseline = contract.get("long_range_baseline")
    if not isinstance(baseline, dict) or baseline.get("id") != "octree-v1":
        errors.append("long_range_baseline must identify octree-v1")
    comparison = contract.get("comparison")
    if not isinstance(comparison, dict) or comparison.get("bvh_gravity_replacement") is not False:
        errors.append("BVH gravity replacement must remain disabled")
    decision = contract.get("decision")
    if not isinstance(decision, dict) or decision.get("status") != "not-selected":
        errors.append("BVH candidate must remain not-selected")
    elif len(decision.get("promotion_requires", [])) != 4:
        errors.append("BVH promotion requirements are incomplete")
    if set(contract.get("required_record_fields", [])) != REQUIRED_FIELDS:
        errors.append("required_record_fields must declare the complete BVH record")
    artifacts = contract.get("artifacts")
    if not isinstance(artifacts, dict) or artifacts.get("generated_outside_source") is not True:
        errors.append("artifacts must require external generated evidence")
    return errors


def _parse_tokens(line: str) -> dict[str, Any]:
    prefix = "BLITZAR BVH "
    if not line.startswith(prefix):
        raise ValueError("unexpected BVH record prefix")
    record: dict[str, Any] = {}
    for token in line[len(prefix) :].split():
        key, separator, value = token.partition("=")
        if not separator or not key or not value or key in record:
            raise ValueError("malformed BVH record")
        if key not in REQUIRED_FIELDS:
            raise ValueError(f"unexpected BVH record field: {key}")
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
    return _parse_tokens(line) if line.startswith("BLITZAR BVH ") else None


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
        return [f"BVH record is missing fields: {missing}"]
    errors = _validate_values(record)
    if set(record) != REQUIRED_FIELDS:
        errors.append("BVH record contains an undeclared field")
    if record["schema"] != contract["record_schema_version"]:
        errors.append("BVH record schema does not match the contract")
    if record["seed"] != contract["seed"] or record["particles"] != contract["particle_count"]:
        errors.append("BVH record identity does not match the contract")
    if record["steps"] != contract["steps"] or record["radius"] != contract["radius"] or record["skin"] != contract["skin"]:
        errors.append("BVH record workload parameters do not match the contract")
    if record["leaf_size"] != contract["leaf_size"] or record["scenario"] not in WORKLOADS:
        errors.append("BVH record scenario or leaf size is invalid")
    expected_refits = 5 if record["scenario"] == "moving" else 0
    expected_rebuilds = 6 if record["scenario"] == "moving" else 1
    if record["bvh_rebuild_count"] != 1 or record["bvh_refit_count"] != expected_refits:
        errors.append("BVH refit pass rebuild counts are invalid")
    if record["bvh_rebuild_baseline_count"] != expected_rebuilds:
        errors.append("BVH full-rebuild baseline count is invalid")
    if record["bvh_build_ns"] <= 0 or record["bvh_query_ns"] <= 0 or record["bvh_rebuild_ns"] <= 0:
        errors.append("BVH build, query, and rebuild timings must be positive")
    if record["cell_linked_build_ns"] <= 0 or record["cell_linked_query_ns"] <= 0:
        errors.append("cell-linked timings must be positive")
    if record["bvh_refit_ns"] <= 0 and record["scenario"] == "moving":
        errors.append("moving BVH refit timing must be positive")
    if (
        record["bvh_neighbor_count"] != record["cell_linked_neighbor_count"]
        or record["bvh_neighbor_count"] != record["reference_count"]
    ):
        errors.append("BVH, cell-linked, and reference counts differ")
    if (
        record["bvh_hash"] != record["cell_linked_hash"]
        or record["bvh_hash"] != record["reference_hash"]
        or record["bvh_ordering_hash"] != record["cell_linked_ordering_hash"]
    ):
        errors.append("BVH, cell-linked, and reference hashes differ")
    if (
        record["bvh_memory_bytes"] <= 0
        or record["bvh_workspace_bytes"] <= 0
        or record["cell_linked_memory_bytes"] <= 0
        or record["octree_build_ns"] <= 0
        or record["octree_cells"] <= 0
        or record["octree_memory_bytes"] <= 0
    ):
        errors.append("BVH, cell-linked, or Octree memory metrics are invalid")
    if not all(record[field] for field in BOOLEAN_FIELDS if field != "selected"):
        errors.append("BVH qualification flags are incomplete")
    if record["selected"] or record["decision"] != "not-selected":
        errors.append("BVH candidate must not be selected")
    return errors


def validate_records(records: list[dict[str, Any]], contract: dict[str, Any]) -> list[str]:
    errors = validate_contract(contract)
    if errors:
        return errors
    if len(records) != len(WORKLOADS):
        errors.append(f"BVH matrix mismatch: expected 4, got {len(records)}")
    seen: set[str] = set()
    for record in records:
        errors.extend(validate_record(record, contract))
        scenario = record.get("scenario")
        if scenario in seen:
            errors.append(f"duplicate BVH record: {scenario}")
        seen.add(scenario)
    if seen != WORKLOADS:
        errors.append("BVH matrix does not cover every workload")
    return errors
