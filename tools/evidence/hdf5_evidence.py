"""Validate and optionally execute the optional HDF5 qualification record."""

from __future__ import annotations

import argparse
import json
import pathlib
import subprocess
import sys


DATASETS = [
    "ids",
    "position_x",
    "position_y",
    "position_z",
    "velocity_x",
    "velocity_y",
    "velocity_z",
    "mass",
]


def load_json(path: pathlib.Path) -> dict[str, object]:
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise ValueError(f"JSON object expected: {path}")
    return value


def validate_contract(root: pathlib.Path) -> list[str]:
    contract = load_json(root / "plan" / "hdf5.json")
    manifest = load_json(root / "plan" / "manifest.json")
    errors: list[str] = []

    if contract.get("schema_version") != 1:
        errors.append("HDF5 contract schema_version must be 1")
    if contract.get("plan_version") != manifest.get("plan_version"):
        errors.append("HDF5 contract plan_version must match the manifest")
    if contract.get("state") != "capability-gated":
        errors.append("HDF5 contract state must remain capability-gated")
    if contract.get("mode_values") != ["AUTO", "ON", "OFF"]:
        errors.append("HDF5 contract mode values are not frozen")
    if contract.get("fallback") != "binary-snapshot-codec":
        errors.append("HDF5 contract must preserve the binary fallback")
    if contract.get("header_attributes") != [
        "schema_version",
        "magic",
        "version",
        "scalar_bytes",
        "particle_count",
        "step",
        "time",
        "rank_count",
        "rank_index",
        "endianness",
        "distribution",
        "id_policy",
        "payload_checksum",
    ]:
        errors.append("HDF5 header attribute order is not frozen")
    if contract.get("payload_datasets") != DATASETS:
        errors.append("HDF5 payload datasets are not frozen")
    if contract.get("payload_order") != DATASETS:
        errors.append("HDF5 payload order must match the dataset order")
    if contract.get("transactional_read") is not True:
        errors.append("HDF5 reads must be transactional")
    if contract.get("atomic_publication") is not True:
        errors.append("HDF5 publication must be atomic")

    determinism = contract.get("determinism")
    if not isinstance(determinism, dict) or determinism.get("same_frame_same_file") is not True:
        errors.append("HDF5 determinism contract is incomplete")
    evidence = contract.get("evidence")
    if not isinstance(evidence, dict) or evidence.get("test_id") != "TST-P6-012":
        errors.append("HDF5 evidence must identify TST-P6-012")

    return errors


def parse_record(output: str) -> dict[str, str]:
    lines = [line.strip() for line in output.splitlines() if line.startswith("hdf5-evidence ")]
    if len(lines) != 1:
        raise ValueError("HDF5 evidence output must contain one record")
    fields = {}
    for item in lines[0].split()[1:]:
        name, separator, value = item.partition("=")
        if not separator or not name or not value:
            raise ValueError("HDF5 evidence fields must use key=value")
        fields[name] = value
    return fields


def _integer(record: dict[str, str], name: str, errors: list[str]) -> int | None:
    try:
        value = int(record[name])
    except (KeyError, ValueError):
        errors.append(f"HDF5 evidence field is not an integer: {name}")
        return None
    if value < 0:
        errors.append(f"HDF5 evidence field is negative: {name}")
    return value


def validate_record(record: dict[str, str], contract: dict[str, object]) -> list[str]:
    required = {
        "backend",
        "schema",
        "soa_fields",
        "repeat_writes",
        "transactional_read",
        "file_bytes",
        "staging_bytes",
        "elapsed_us",
    }
    errors = [f"HDF5 evidence field is missing: {name}" for name in sorted(required - set(record))]
    if errors:
        return errors
    if record["backend"] not in {"enabled", "unavailable"}:
        errors.append("HDF5 evidence backend is invalid")
    if record["schema"] != "1" or record["soa_fields"] != str(len(DATASETS)):
        errors.append("HDF5 evidence schema fields are invalid")
    qualification = contract.get("determinism", {})
    expected_repeat = qualification.get("qualification_repeat_writes")
    if record["repeat_writes"] != str(expected_repeat):
        errors.append("HDF5 evidence repeat count does not match the contract")
    if record["transactional_read"] != "1":
        errors.append("HDF5 evidence does not prove transactional reads")

    file_bytes = _integer(record, "file_bytes", errors)
    staging_bytes = _integer(record, "staging_bytes", errors)
    _integer(record, "elapsed_us", errors)
    if record["backend"] == "unavailable" and file_bytes != 0:
        errors.append("unavailable HDF5 evidence must not publish a file")
    if record["backend"] == "enabled" and (file_bytes is None or file_bytes <= 0):
        errors.append("enabled HDF5 evidence must publish a non-empty file")
    if record["backend"] == "enabled" and staging_bytes != 128:
        errors.append("HDF5 staging evidence must match the two-particle fixture")
    return errors


def run_binary(binary: pathlib.Path) -> dict[str, str]:
    result = subprocess.run([str(binary)], check=False, capture_output=True, text=True)
    if result.returncode != 0:
        raise RuntimeError(
            f"HDF5 qualification binary failed with {result.returncode}: {result.stderr.strip()}"
        )
    return parse_record(result.stdout)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=pathlib.Path, default=pathlib.Path(__file__).resolve().parents[2])
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--binary", type=pathlib.Path)
    parser.add_argument("--output", type=pathlib.Path)
    arguments = parser.parse_args(argv)
    root = arguments.root.resolve()

    try:
        contract_errors = validate_contract(root)
        if contract_errors:
            for error in contract_errors:
                print(f"hdf5-evidence: {error}", file=sys.stderr)
            return 1
        if arguments.check and arguments.binary is None:
            print("hdf5-evidence: contract is valid")
            return 0
        if arguments.binary is None:
            parser.error("--binary is required unless --check is used")
        record = run_binary(arguments.binary.resolve())
        contract = load_json(root / "plan" / "hdf5.json")
        record_errors = validate_record(record, contract)
        if record_errors:
            for error in record_errors:
                print(f"hdf5-evidence: {error}", file=sys.stderr)
            return 1
        rendered = {
            "schema_version": 1,
            "plan_version": contract["plan_version"],
            "record": record,
        }
        if arguments.output is not None:
            output = arguments.output.resolve()
            if root == output or root in output.parents:
                raise ValueError("HDF5 evidence output must be outside the source tree")
            output.parent.mkdir(parents=True, exist_ok=True)
            output.write_text(json.dumps(rendered, indent=2) + "\n", encoding="utf-8")
        print("hdf5-evidence: runtime record is valid")
        return 0
    except (OSError, RuntimeError, ValueError, KeyError, TypeError, json.JSONDecodeError) as error:
        print(f"hdf5-evidence: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
