"""Validate the deterministic Grid, PM, and TreePM qualification contract."""

from __future__ import annotations

import json
import math
import pathlib
from typing import Any


SCHEMA_VERSION = 1
ROOTS = ["src/grid", "src/solvers/pm", "src/solvers/treepm"]
TESTS = ["TST-P5-001", "TST-P5-002", "TST-P5-003"]


def load_contract(root: pathlib.Path) -> dict[str, Any]:
    value = json.loads((root / "plan" / "grid.json").read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise ValueError("grid contract must be a JSON object")
    return value


def validate_contract(contract: dict[str, Any]) -> list[str]:
    errors: list[str] = []
    if contract.get("schema_version") != SCHEMA_VERSION:
        errors.append("schema_version must be 1")
    if not isinstance(contract.get("plan_version"), str):
        errors.append("plan_version must be a string")
    if contract.get("phase") != "P5":
        errors.append("phase must be P5")
    if contract.get("roots") != ROOTS:
        errors.append("roots must list Grid, PM, and TreePM in order")
    if contract.get("precision") != "float64":
        errors.append("precision must remain float64")
    reference = contract.get("reference")
    if reference != {
        "solver": "direct",
        "backend": "cpu",
        "comparison": "force vectors and deterministic KDK state",
    }:
        errors.append("Direct CPU reference contract is incomplete")
    errors.extend(_validate_grid(contract.get("grid")))
    errors.extend(_validate_solvers(contract.get("solvers")))
    errors.extend(_validate_distribution(contract.get("distributed")))
    errors.extend(_validate_lifecycle(contract.get("lifecycle")))
    errors.extend(_validate_acceptance(contract.get("acceptance")))
    if contract.get("public_api") != {
        "changed": False,
        "solver_values": "existing BLITZAR_SOLVER_PM and BLITZAR_SOLVER_TREEPM values",
        "new_public_settings": False,
    }:
        errors.append("public API contract must remain unchanged")
    artifacts = contract.get("artifacts")
    if not isinstance(artifacts, dict) or artifacts.get("generated_outside_source") is not True:
        errors.append("artifacts must require external generated evidence")
    return errors


def _validate_grid(value: Any) -> list[str]:
    if not isinstance(value, dict):
        return ["grid must be an object"]
    errors: list[str] = []
    if value.get("dimensions") != [8, 8, 8]:
        errors.append("grid dimensions must remain [8, 8, 8]")
    expected = {
        "capacity": "simulation particle capacity",
        "boundary": "finite non-periodic; coordinates outside the domain are clamped to the nearest cell",
        "deposition": "cloud-in-cell with mass-conserving trilinear weights",
        "field": "softened discrete Newton Green convolution at cell centers",
        "interpolation": "trilinear field interpolation with the same clamped cell policy",
        "ordering": "z-major flat cells, then ascending source index",
    }
    for key, expected_value in expected.items():
        if value.get(key) != expected_value:
            errors.append(f"grid {key} policy is invalid")
    if value.get("domain") != (
        "source AABB expanded by one length unit on every side; degenerate axes use a two-unit span centered on the source coordinate"
    ):
        errors.append("grid domain policy must be declared")
    return errors


def _validate_solvers(value: Any) -> list[str]:
    if not isinstance(value, dict) or set(value) != {"pm", "treepm"}:
        return ["solvers must contain PM and TreePM"]
    errors: list[str] = []
    pm = value["pm"]
    treepm = value["treepm"]
    if pm != {
        "id": "pm-cpu-v1",
        "resource": "grid",
        "scope": "single-rank CPU",
        "selected": True,
    }:
        errors.append("PM solver policy is invalid")
    if treepm != {
        "id": "treepm-cpu-v1",
        "resources": ["octree", "grid"],
        "composition": "0.5 PM long-range field plus 0.5 Barnes-Hut field",
        "scope": "single-rank CPU",
        "selected": True,
    }:
        errors.append("TreePM solver policy is invalid")
    return errors


def _validate_distribution(value: Any) -> list[str]:
    expected = {
        "status": "unsupported",
        "reason": "P5 does not define a global mesh ownership or halo contract",
        "preflight": "reject before transaction, migration, or state mutation",
    }
    return [] if value == expected else ["distributed mesh policy is invalid"]


def _validate_lifecycle(value: Any) -> list[str]:
    if not isinstance(value, dict):
        return ["lifecycle must be an object"]
    required = {"initialization", "preparation", "steady_state", "failure"}
    if set(value) != required or any(not isinstance(item, str) or not item for item in value.values()):
        return ["lifecycle must declare all bounded allocation phases"]
    if "allocate bounded grid storage" not in value["initialization"]:
        return ["grid storage must be allocated during initialization"]
    if "zero dynamic allocations" not in value["steady_state"]:
        return ["steady-state allocation policy is invalid"]
    return []


def _validate_acceptance(value: Any) -> list[str]:
    if not isinstance(value, dict):
        return ["acceptance must be an object"]
    if value.get("tests") != TESTS:
        return ["acceptance tests must list the three P5 tests in order"]
    if value.get("checks") != ["CHK-P0-046", "CHK-P0-047"]:
        return ["acceptance checks must list the two P5 contract checks"]
    tolerance = value.get("relative_force_l2_error_max")
    if not isinstance(tolerance, (int, float)) or not math.isfinite(tolerance) or tolerance != 4.0:
        return ["relative force tolerance must remain 4.0"]
    return []
