"""Validate the deterministic Morton and particle-layout evidence contract."""

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
    "particles",
    "tile_width",
    "sort_ns",
    "materialize_ns",
    "tree_build_ns",
    "scan_ns",
    "cache_line_visits_proxy",
    "candidate_bytes",
    "materialized_bytes",
    "order_hash",
    "state_hash",
    "byte_hash",
    "tree_hash",
}
FLOAT_FIELDS = {
    "scan_particles_per_second",
    "locality_mean_squared_distance",
    "scan_checksum",
}
BOOLEAN_FIELDS = {
    "stable",
    "repeatable",
    "ordering_equivalent",
    "representation_equivalent",
    "tree_valid",
}


def load_contract(root: pathlib.Path) -> dict[str, Any]:
    value = json.loads((root / "plan" / "layout.json").read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise ValueError("layout contract must be a JSON object")
    return value


def _is_integer(value: Any) -> bool:
    return isinstance(value, int) and not isinstance(value, bool)


def validate_contract(contract: dict[str, Any]) -> list[str]:
    errors: list[str] = []
    if contract.get("schema_version") != SCHEMA_VERSION:
        errors.append(f"schema_version must be {SCHEMA_VERSION}")
    if contract.get("record_schema_version") != RECORD_SCHEMA_VERSION:
        errors.append(f"record_schema_version must be {RECORD_SCHEMA_VERSION}")
    if not isinstance(contract.get("plan_version"), str):
        errors.append("plan_version must be a string")
    if contract.get("target") != "blitzar_layout_test":
        errors.append("target must be blitzar_layout_test")
    if contract.get("runner") != "tools/evidence/layout_evidence.py":
        errors.append("runner must be tools/evidence/layout_evidence.py")
    if not _is_integer(contract.get("seed")) or contract["seed"] < 0:
        errors.append("seed must be a non-negative integer")
    counts = contract.get("particle_counts")
    if not isinstance(counts, list) or not counts or any(
        not _is_integer(item) or item <= 0 for item in counts
    ):
        errors.append("particle_counts must contain positive integers")
    elif len(set(counts)) != len(counts):
        errors.append("particle_counts must be unique")
    widths = contract.get("bounded_tile_widths")
    if widths != [4, 8, 16, 32]:
        errors.append("bounded_tile_widths must remain [4, 8, 16, 32]")
    errors.extend(_validate_orderings(contract.get("orderings")))
    errors.extend(_validate_representations(contract.get("representations")))
    errors.extend(_validate_metrics(contract.get("metrics")))
    fields = contract.get("required_record_fields")
    expected_fields = INTEGER_FIELDS | FLOAT_FIELDS | BOOLEAN_FIELDS | {"ordering", "layout"}
    expected_fields.update({"schema", "seed", "particles", "tile_width"})
    if not isinstance(fields, list) or set(fields) != expected_fields:
        errors.append("required_record_fields must declare the complete record")
    selected = contract.get("selected_boundaries")
    if not isinstance(selected, dict) or selected.get("octree") != "soa-v1" or selected.get(
        "grid"
    ) != "soa-v1":
        errors.append("Octree and grid boundaries must select soa-v1")
    artifacts = contract.get("artifacts")
    if not isinstance(artifacts, dict) or artifacts.get("generated_outside_source") is not True:
        errors.append("artifacts must require external generated evidence")
    return errors


def _validate_orderings(value: Any) -> list[str]:
    if not isinstance(value, list) or len(value) != 2:
        return ["orderings must contain exactly two candidates"]
    errors: list[str] = []
    identifiers: set[str] = set()
    for item in value:
        if not isinstance(item, dict) or not isinstance(item.get("id"), str):
            errors.append("every ordering needs an id")
            continue
        identifier = item["id"]
        if identifier in identifiers:
            errors.append(f"duplicate ordering id: {identifier}")
        identifiers.add(identifier)
        if not isinstance(item.get("algorithm"), str) or not item["algorithm"]:
            errors.append(f"ordering {identifier} needs an algorithm")
    if identifiers != {"stable-comparison-v1", "stable-radix-v1"}:
        errors.append("orderings must contain the comparison and radix candidates")
    return errors


def _validate_representations(value: Any) -> list[str]:
    if not isinstance(value, list) or len(value) != 5:
        return ["representations must contain SoA and four bounded AoSoA candidates"]
    errors: list[str] = []
    identifiers: set[str] = set()
    expected = {"soa-v1", "aosoa-4-v1", "aosoa-8-v1", "aosoa-16-v1", "aosoa-32-v1"}
    for item in value:
        if not isinstance(item, dict) or not isinstance(item.get("id"), str):
            errors.append("every representation needs an id")
            continue
        identifier = item["id"]
        if identifier in identifiers:
            errors.append(f"duplicate representation id: {identifier}")
        identifiers.add(identifier)
        if item.get("kind") not in {"soa", "aosoa"} or not _is_integer(
            item.get("tile_width")
        ):
            errors.append(f"representation {identifier} has invalid layout data")
        if not isinstance(item.get("selected"), bool):
            errors.append(f"representation {identifier} needs a selection flag")
    if identifiers != expected:
        errors.append("representations do not match the bounded candidate set")
    return errors


def _validate_metrics(value: Any) -> list[str]:
    if not isinstance(value, dict):
        return ["metrics must be an object"]
    required = {
        "morton_sort_time_ns",
        "materialization_time_ns",
        "octree_build_time_ns",
        "contiguous_scan_time_ns",
        "scan_particles_per_second",
        "locality_mean_squared_distance",
        "cache_line_visits",
        "candidate_memory_bytes",
        "materialized_memory_bytes",
        "ordering_hash",
        "logical_state_hash",
        "candidate_byte_hash",
        "octree_hash",
        "byte_repeatability",
    }
    return [] if set(value) == required else ["metrics must declare the complete mapping"]


def parse_layout_record(line: str) -> dict[str, Any] | None:
    if not line.startswith("BLITZAR LAYOUT "):
        return None
    record: dict[str, Any] = {}
    for token in line.split()[2:]:
        key, separator, value = token.partition("=")
        if not separator or key in record:
            raise ValueError("malformed layout record")
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


def representation_id(record: dict[str, Any]) -> str | None:
    if record.get("layout") == "soa" and record.get("tile_width") == 0:
        return "soa-v1"
    if record.get("layout") == "aosoa" and record.get("tile_width") in {4, 8, 16, 32}:
        return f"aosoa-{record['tile_width']}-v1"
    return None


def validate_record(record: dict[str, Any], contract: dict[str, Any]) -> list[str]:
    required = set(contract["required_record_fields"])
    missing = sorted(required - set(record))
    if missing:
        return [f"record is missing fields: {missing}"]
    errors: list[str] = []
    if record["schema"] != contract["record_schema_version"]:
        errors.append("record schema does not match the contract")
    if record["seed"] != contract["seed"]:
        errors.append("record seed does not match the contract")
    if record["particles"] not in contract["particle_counts"]:
        errors.append("record particle count is not declared")
    ordering_ids = {item["id"] for item in contract["orderings"]}
    if record["ordering"] not in ordering_ids:
        errors.append("record ordering is not declared")
    if representation_id(record) not in {
        item["id"] for item in contract["representations"]
    }:
        errors.append("record representation is not declared")
    for field in INTEGER_FIELDS:
        if not _is_integer(record[field]) or record[field] < 0:
            errors.append(f"record integer field is invalid: {field}")
    for field in FLOAT_FIELDS:
        if not isinstance(record[field], (int, float)) or not math.isfinite(record[field]):
            errors.append(f"record float field is invalid: {field}")
    for field in ("sort_ns", "materialize_ns", "tree_build_ns", "scan_ns"):
        if record[field] <= 0:
            errors.append(f"record timing is not positive: {field}")
    if not all(record[field] for field in BOOLEAN_FIELDS):
        errors.append("record qualification flags are not all true")
    return errors


def validate_records(records: list[dict[str, Any]], contract: dict[str, Any]) -> list[str]:
    errors = validate_contract(contract)
    if errors:
        return errors
    expected = {
        (count, ordering["id"], representation["id"])
        for count in contract["particle_counts"]
        for ordering in contract["orderings"]
        for representation in contract["representations"]
    }
    seen: set[tuple[Any, ...]] = set()
    for record in records:
        errors.extend(validate_record(record, contract))
        identifier = (record.get("particles"), record.get("ordering"), representation_id(record))
        if identifier in seen:
            errors.append(f"duplicate layout record: {identifier}")
        seen.add(identifier)
    if seen != expected:
        errors.append(f"layout matrix mismatch: expected {len(expected)}, got {len(seen)}")
    for count in contract["particle_counts"]:
        group = [record for record in records if record.get("particles") == count]
        if len({record.get("state_hash") for record in group}) != 1:
            errors.append(f"logical state hash differs for particle count {count}")
        if len({record.get("tree_hash") for record in group}) != 1:
            errors.append(f"Octree hash differs for particle count {count}")
        if len({record.get("order_hash") for record in group}) != 1:
            errors.append(f"ordering hash differs for particle count {count}")
    return errors
