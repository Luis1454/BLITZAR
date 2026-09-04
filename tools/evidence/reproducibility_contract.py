"""Validate the frozen execution-policy and restart-state contract."""

from __future__ import annotations

import argparse
import json
import pathlib
import sys
from typing import Any


SCHEMA_VERSION = 1


def load_contract(root: pathlib.Path) -> dict[str, Any]:
    value = json.loads(
        (root / "plan" / "reproducibility.json").read_text(encoding="utf-8")
    )
    if not isinstance(value, dict):
        raise ValueError("reproducibility contract must be a JSON object")
    return value


def _expected_contract() -> dict[str, Any]:
    return {
        "schema_version": SCHEMA_VERSION,
        "phase": "P6",
        "issue": "#685",
        "public_api": {
            "changed": False,
            "c_abi": "unchanged",
            "cpp_wrapper": "unchanged",
            "policy_boundary": "internal execution settings and CLI configuration",
        },
        "configuration": {
            "directive": "execution",
            "arguments": [
                {"name": "mode", "type": "enum(strict|fast)", "required": True}
            ],
            "optional": True,
            "default": "strict",
        },
        "modes": {
            "strict": {
                "fma": "disabled",
                "reduction": "ordered",
                "bitwise_reproducible": True,
                "scope": "same backend, compiler identity, device identity, and plan version",
                "restart": "step 0 to 100 equals step 0 to 50 save/reload/resume to 100",
            },
            "fast": {
                "fma": "hardware",
                "reduction": "backend",
                "bitwise_reproducible": False,
                "scope": "tolerance comparison only",
                "restart": "numerical tolerance is required; bitwise equality is not advertised",
            },
        },
        "backend_policy_order": ["cpu", "hip", "mpi"],
        "backend_policy_fields": ["fma", "reduction"],
        "restart_state": {
            "integrator": "leapfrog_kdk",
            "rng": "seeded-jitter-v1",
            "compensator": "direct-plain;diagnostics-neumaier-v1",
            "ordering": "stable-particle-id-v1",
            "units": ["length_scale", "mass_scale", "time_scale"],
            "precision": "float64",
            "compiler": "exact compiler family and version identity",
            "device": "selected execution boundary identity",
            "math_mode": "execution mode plus per-backend FMA and reduction policies",
            "seed": "generation.seed",
        },
        "snapshot_boundary": {
            "mpi_in_flight": "forbidden",
            "required_state": [
                "integrator",
                "rng",
                "compensator",
                "ordering",
                "units",
                "precision",
                "compiler",
                "device",
                "math_mode",
            ],
            "publication": "capture only after the simulation reports no active ghost exchange",
        },
        "cross_backend": {
            "reference": "direct CPU solver",
            "comparison": "finite state and force tolerances",
            "bitwise_policy": "same-backend strict only",
            "gpu_device": "capability-gated",
        },
        "acceptance": {
            "tests": ["TST-P6-009", "TST-P6-015"],
            "checks": ["CHK-P0-048", "CHK-P0-049"],
            "strict_restart": "byte-identical final snapshot and compatible manifest",
            "fast_disclosure": "metadata bitwise_reproducible is false",
            "boundary": "active MPI ghost exchange rejects snapshot capture",
        },
        "artifacts": {"generated_outside_source": True, "wall_clock_metadata": False},
    }


def validate_contract(contract: dict[str, Any]) -> list[str]:
    errors: list[str] = []
    expected = _expected_contract()

    for key, value in expected.items():
        if key == "schema_version":
            actual = contract.get(key)
            if actual != value:
                errors.append(f"schema_version must be {SCHEMA_VERSION}")
        elif contract.get(key) != value:
            errors.append(f"{key} contract is incomplete or inconsistent")

    if not isinstance(contract.get("plan_version"), str):
        errors.append("plan_version must be a string")

    return errors


def validate_plan_version(root: pathlib.Path, contract: dict[str, Any]) -> list[str]:
    manifest = json.loads(
        (root / "plan" / "manifest.json").read_text(encoding="utf-8")
    )
    if contract.get("plan_version") != manifest.get("plan_version"):
        return ["contract plan_version must match plan/manifest.json"]
    return []


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=pathlib.Path, required=True)
    parser.add_argument("--check", action="store_true")
    arguments = parser.parse_args(argv)
    root = arguments.root.resolve()

    try:
        contract = load_contract(root)
        errors = validate_contract(contract) + validate_plan_version(root, contract)
    except (OSError, ValueError, json.JSONDecodeError) as error:
        print(f"reproducibility-contract: {error}", file=sys.stderr)
        return 1

    if errors:
        for error in errors:
            print(f"reproducibility-contract: {error}", file=sys.stderr)
        return 1

    print(f"reproducibility-contract: {contract['plan_version']} is valid")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
