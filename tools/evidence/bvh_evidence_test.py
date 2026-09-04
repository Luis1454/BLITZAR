"""Test the BVH evidence contract and parser with deterministic fixtures."""

from __future__ import annotations

import pathlib

from tools.evidence.bvh_contract import (
    BOOLEAN_FIELDS,
    INTEGER_FIELDS,
    STRING_FIELDS,
    FLOAT_FIELDS,
    load_contract,
    parse_record,
    validate_contract,
    validate_record,
    validate_records,
)


ROOT = pathlib.Path(__file__).resolve().parents[2]


def fixture(scenario: str) -> dict[str, object]:
    moving = scenario == "moving"
    record: dict[str, object] = {
        "schema": 1,
        "seed": 424242,
        "scenario": scenario,
        "particles": 96,
        "steps": 6,
        "radius": 0.75,
        "skin": 0.4,
        "leaf_size": 4,
        "bvh_build_ns": 10,
        "bvh_refit_ns": 10 if moving else 0,
        "bvh_query_ns": 20,
        "bvh_rebuild_ns": 30,
        "cell_linked_build_ns": 10,
        "cell_linked_query_ns": 20,
        "bvh_rebuild_count": 1,
        "bvh_refit_count": 5 if moving else 0,
        "bvh_rebuild_baseline_count": 6 if moving else 1,
        "bvh_neighbor_count": 12,
        "cell_linked_neighbor_count": 12,
        "reference_count": 12,
        "bvh_memory_bytes": 1024,
        "bvh_workspace_bytes": 256,
        "cell_linked_memory_bytes": 1024,
        "bvh_hash": 77,
        "bvh_topology_hash": 88,
        "cell_linked_hash": 77,
        "reference_hash": 77,
        "bvh_ordering_hash": 99,
        "cell_linked_ordering_hash": 99,
        "octree_build_ns": 10,
        "octree_cells": 4,
        "octree_memory_bytes": 2048,
        "octree_hash": 66,
        "finite": True,
        "correct": True,
        "repeatable": True,
        "deterministic": True,
        "refit_correct": True,
        "rebuild_correct": True,
        "refit_parity": True,
        "selected": False,
        "decision": "not-selected",
    }
    return record


def render(record: dict[str, object]) -> str:
    values: list[str] = []
    for key, value in record.items():
        if key in BOOLEAN_FIELDS:
            values.append(f"{key}={1 if value else 0}")
        else:
            values.append(f"{key}={value}")
    return "BLITZAR BVH " + " ".join(values)


def main() -> int:
    contract = load_contract(ROOT)
    assert not validate_contract(contract)
    records = []
    for scenario in sorted({"dense", "sparse", "clustered", "moving"}):
        record = fixture(scenario)
        assert not validate_record(record, contract)
        parsed = parse_record(render(record))
        assert parsed is not None
        assert not validate_record(parsed, contract)
        records.append(parsed)
    assert not validate_records(records, contract)

    rejected = fixture("dense")
    rejected["selected"] = True
    assert validate_record(rejected, contract)
    assert INTEGER_FIELDS and FLOAT_FIELDS and BOOLEAN_FIELDS and STRING_FIELDS
    print("bvh-evidence-test: fixtures passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
