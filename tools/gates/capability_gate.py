"""Validate the frozen implementation and backend capability contract."""

from __future__ import annotations

import argparse
import json
import pathlib
import re
import sys


STATES = {
    "implemented-qualified",
    "implemented-local",
    "capability-gated",
    "unsupported",
    "deferred",
}
SOLVERS = {"direct": 0, "barnes-hut": 1, "fmm": 2, "pm": 3, "treepm": 4}
BACKENDS = {"cpu", "hip", "mpi"}
DEFERRED_ROOTS = {"src/grid", "src/io", "src/solvers/pm", "src/solvers/treepm"}


def load_json(path: pathlib.Path) -> object:
    return json.loads(path.read_text(encoding="utf-8"))


def validate_state(state: object) -> list[str]:
    if state not in STATES:
        return [f"unknown capability state: {state}"]
    return []


def validate_matrix(root: pathlib.Path) -> list[str]:
    matrix = load_json(root / "plan" / "capabilities.json")
    manifest = load_json(root / "plan" / "manifest.json")
    if not isinstance(matrix, dict) or not isinstance(manifest, dict):
        return ["capability matrix and manifest must be objects"]

    errors: list[str] = []
    source_of_truth = manifest.get("source_of_truth", [])
    if "plan/capabilities.json" not in source_of_truth:
        errors.append("capability matrix must be a manifest source of truth")

    solver_entries = matrix.get("solvers")
    if not isinstance(solver_entries, list):
        errors.append("capability matrix needs a solver list")
        solver_entries = []
    solver_ids: set[str] = set()
    solver_values: set[int] = set()
    for entry in solver_entries:
        if not isinstance(entry, dict):
            errors.append("solver capability entries must be objects")
            continue
        solver_id = entry.get("id")
        public_value = entry.get("public_value")
        solver_ids.add(str(solver_id))
        if not isinstance(public_value, int):
            errors.append(f"solver {solver_id} needs an integer public value")
        else:
            solver_values.add(public_value)
        errors.extend(validate_state(entry.get("state")))
        if not isinstance(entry.get("compile_requirement"), str):
            errors.append(f"solver {solver_id} needs a compile requirement")
        if not isinstance(entry.get("runtime_requirement"), str):
            errors.append(f"solver {solver_id} needs a runtime requirement")
        evidence = entry.get("runtime_evidence")
        if not isinstance(evidence, list):
            errors.append(f"solver {solver_id} needs runtime evidence metadata")
        elif entry.get("state") != "deferred" and not evidence:
            errors.append(f"solver {solver_id} claims a state without evidence")
    if solver_ids != set(SOLVERS):
        errors.append(f"solver capability ids must be {sorted(SOLVERS)}")
    if solver_values != set(SOLVERS.values()):
        errors.append("solver capability values must cover the public enum")

    feature_entries = matrix.get("features")
    if not isinstance(feature_entries, list):
        errors.append("capability matrix needs a feature list")
        feature_entries = []
    for entry in feature_entries:
        if not isinstance(entry, dict):
            errors.append("feature capability entries must be objects")
            continue
        errors.extend(validate_state(entry.get("state")))
        if not isinstance(entry.get("runtime_evidence"), list):
            errors.append(f"feature {entry.get('id')} needs runtime evidence metadata")

    backend_entries = matrix.get("backends")
    if not isinstance(backend_entries, list):
        errors.append("capability matrix needs a backend list")
        backend_entries = []
    backend_ids: set[str] = set()
    for entry in backend_entries:
        if not isinstance(entry, dict):
            errors.append("backend capability entries must be objects")
            continue
        backend_id = str(entry.get("id"))
        backend_ids.add(backend_id)
        errors.extend(validate_state(entry.get("state")))
        if entry.get("compile_support") not in {"required", "optional"}:
            errors.append(f"backend {backend_id} needs required or optional compile support")
        if not isinstance(entry.get("requirements"), list) or not entry["requirements"]:
            errors.append(f"backend {backend_id} needs runtime requirements")
        if not isinstance(entry.get("runtime_evidence"), list) or not entry["runtime_evidence"]:
            errors.append(f"backend {backend_id} needs runtime evidence metadata")
    if backend_ids != BACKENDS:
        errors.append(f"backend capability ids must be {sorted(BACKENDS)}")

    deferred_roots = set(manifest.get("deferred_roots", []))
    if deferred_roots != DEFERRED_ROOTS:
        errors.append("manifest deferred roots no longer match the capability contract")
    for relative in sorted(DEFERRED_ROOTS):
        if (root / relative).exists():
            errors.append(f"deferred production root is materialized: {relative}")

    public_report = matrix.get("public_report")
    if not isinstance(public_report, dict):
        errors.append("capability matrix needs a public report contract")
    else:
        function_name = str(public_report.get("function", ""))
        fields = public_report.get("compile_only_fields")
        if function_name != "blitzar_get_capabilities_v2":
            errors.append("public capability report function is not frozen")
        if not isinstance(fields, list) or set(fields) != {
            "implemented_solver_mask",
            "unsupported_solver_mask",
            "deferred_feature_mask",
            "compiled_backend_mask",
        }:
            errors.append("public capability report fields are incomplete")

    header = (root / "include" / "blitzar" / "blitzar.h").read_text(encoding="utf-8")
    api_info = (root / "src" / "sdk" / "c" / "ApiInfo.cpp").read_text(encoding="utf-8")
    config = (
        root / "src" / "simulation" / "configuration" / "SimulationConfig.cpp"
    ).read_text(encoding="utf-8")
    plan = (root / "PLAN.md").read_text(encoding="utf-8")
    required_public = ("blitzar_capabilities_v2", "blitzar_get_capabilities_v2")
    errors.extend(
        f"public capability symbol is missing: {symbol}"
        for symbol in required_public
        if symbol not in header
    )
    if "blitzar_get_capabilities_v2" not in api_info:
        errors.append("public capability function has no implementation")
    if not re.search(
        r"case BLITZAR_SOLVER_PM:.*?case BLITZAR_SOLVER_TREEPM:.*?"
        r"return BLITZAR_STATUS_UNSUPPORTED;",
        config,
        re.DOTALL,
    ):
        errors.append("PM and TreePM are not visibly rejected as unsupported")
    if "SnapshotHeader" not in plan or "contract hook" not in plan:
        errors.append("PLAN.md must describe SnapshotHeader as a contract hook")

    return errors


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=pathlib.Path, required=True)
    parser.add_argument("--check", action="store_true")
    arguments = parser.parse_args(argv)
    root = arguments.root.resolve()
    try:
        errors = validate_matrix(root)
    except (OSError, KeyError, TypeError, ValueError, json.JSONDecodeError) as error:
        print(f"capability-gate: {error}", file=sys.stderr)
        return 1
    if errors:
        for error in errors:
            print(f"capability-gate: {error}", file=sys.stderr)
        return 1
    print("capability-gate: compile/runtime capability contract is consistent")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
