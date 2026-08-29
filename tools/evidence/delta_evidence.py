"""Validate and optionally execute the deterministic snapshot delta qualification."""

from __future__ import annotations

import argparse
import json
import pathlib
import subprocess
import sys


PAYLOAD_FIELDS = [
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
    contract = load_json(root / "plan" / "delta.json")
    manifest = load_json(root / "plan" / "manifest.json")
    errors: list[str] = []

    if contract.get("schema_version") != 1:
        errors.append("delta contract schema_version must be 1")
    if contract.get("plan_version") != manifest.get("plan_version"):
        errors.append("delta contract plan_version must match the manifest")
    if contract.get("state") != "evaluated-not-default":
        errors.append("delta contract must remain non-default")
    if contract.get("codec") != "xor-rle-v1":
        errors.append("delta codec identifier is not frozen")
    if contract.get("scope") != ["snapshot-payload", "transport-payload"]:
        errors.append("delta scope is not frozen")
    if contract.get("payload_order") != PAYLOAD_FIELDS:
        errors.append("delta payload order must match the binary payload")
    if contract.get("max_particle_count") != 100000:
        errors.append("delta maximum particle count is not frozen")
    if contract.get("max_encoded_bytes") != "24 + 2 * raw_payload_bytes":
        errors.append("delta maximum encoded size policy is not frozen")
    if contract.get("checksum") != "FNV-1a-64 over current canonical payload":
        errors.append("delta checksum policy is not frozen")
    if contract.get("transactional_decode") is not True:
        errors.append("delta decode must be transactional")
    if contract.get("fallback") != "binary-snapshot-codec":
        errors.append("delta fallback must remain the binary codec")
    random_access = contract.get("random_access")
    if not isinstance(random_access, dict):
        errors.append("delta random-access policy is missing")
    else:
        if random_access.get("keyframe_interval") != 8:
            errors.append("delta keyframe interval is not frozen")
        if random_access.get("maximum_replay_deltas") != 7:
            errors.append("delta maximum replay policy is not frozen")
        if random_access.get("default_restart") != "binary-snapshot-codec":
            errors.append("delta default restart fallback is not frozen")
    evidence = contract.get("evidence")
    if not isinstance(evidence, dict) or evidence.get("test_id") != "TST-P6-013":
        errors.append("delta evidence must identify TST-P6-013")

    return errors


def parse_record(output: str) -> dict[str, str]:
    lines = [line.strip() for line in output.splitlines() if line.startswith("delta-evidence ")]
    if len(lines) != 1:
        raise ValueError("delta evidence output must contain one record")
    fields: dict[str, str] = {}
    for item in lines[0].split()[1:]:
        name, separator, value = item.partition("=")
        if not separator or not name or not value or name in fields:
            raise ValueError("delta evidence fields must use unique key=value pairs")
        fields[name] = value
    return fields


def integer(record: dict[str, str], name: str, errors: list[str]) -> int | None:
    try:
        value = int(record[name])
    except (KeyError, ValueError):
        errors.append(f"delta evidence field is not an integer: {name}")
        return None
    if value < 0:
        errors.append(f"delta evidence field is negative: {name}")
    return value


def validate_record(record: dict[str, str], contract: dict[str, object]) -> list[str]:
    required = {
        "backend",
        "codec",
        "scope",
        "raw_bytes",
        "encoded_bytes",
        "ratio_ppm",
        "reference_write_ns",
        "reference_read_ns",
        "delta_write_ns",
        "delta_read_ns",
        "workspace_bytes",
        "keyframe_interval",
        "random_access",
        "checksum",
        "deterministic",
        "corruption_rejected",
        "transactional",
        "transport",
        "fallback",
    }
    errors = [f"delta evidence field is missing: {name}" for name in sorted(required - set(record))]
    if errors:
        return errors
    if record["backend"] != "cpu" or record["codec"] != contract.get("codec"):
        errors.append("delta evidence backend or codec is invalid")
    if record["scope"] != "snapshot-transport-payload":
        errors.append("delta evidence scope is invalid")
    if record["keyframe_interval"] != str(contract["random_access"]["keyframe_interval"]):
        errors.append("delta evidence keyframe interval does not match the contract")
    if record["random_access"] != "index-replay":
        errors.append("delta evidence random-access policy is invalid")
    if record["fallback"] != contract["fallback"]:
        errors.append("delta evidence fallback is invalid")
    for name in (
        "raw_bytes",
        "encoded_bytes",
        "ratio_ppm",
        "reference_write_ns",
        "reference_read_ns",
        "delta_write_ns",
        "delta_read_ns",
        "workspace_bytes",
        "checksum",
    ):
        integer(record, name, errors)
    raw_bytes = integer(record, "raw_bytes", errors)
    encoded_bytes = integer(record, "encoded_bytes", errors)
    ratio_ppm = integer(record, "ratio_ppm", errors)
    if raw_bytes is not None and raw_bytes <= 0:
        errors.append("delta evidence raw payload must be non-empty")
    if encoded_bytes is not None and raw_bytes is not None:
        if encoded_bytes <= 0 or encoded_bytes >= raw_bytes:
            errors.append("delta evidence must demonstrate a smaller stream")
    if raw_bytes and encoded_bytes is not None and ratio_ppm is not None:
        if ratio_ppm != encoded_bytes * 1_000_000 // raw_bytes:
            errors.append("delta evidence ratio is inconsistent")
    if record["deterministic"] != "1" or record["corruption_rejected"] != "1":
        errors.append("delta evidence does not prove deterministic corruption rejection")
    if record["transactional"] != "1" or record["transport"] != "1":
        errors.append("delta evidence does not prove transactional transport reuse")
    return errors


def run_binary(binary: pathlib.Path) -> dict[str, str]:
    result = subprocess.run([str(binary)], check=False, capture_output=True, text=True)
    if result.returncode != 0:
        raise RuntimeError(
            f"delta qualification binary failed with {result.returncode}: {result.stderr.strip()}"
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
                print(f"delta-evidence: {error}", file=sys.stderr)
            return 1
        if arguments.check and arguments.binary is None:
            print("delta-evidence: contract is valid")
            return 0
        if arguments.binary is None:
            parser.error("--binary is required unless --check is used")
        contract = load_json(root / "plan" / "delta.json")
        record = run_binary(arguments.binary.resolve())
        record_errors = validate_record(record, contract)
        if record_errors:
            for error in record_errors:
                print(f"delta-evidence: {error}", file=sys.stderr)
            return 1
        rendered = {
            "schema_version": 1,
            "plan_version": contract["plan_version"],
            "record": record,
        }
        if arguments.output is not None:
            output = arguments.output.resolve()
            if output == root or root in output.parents:
                raise ValueError("delta evidence output must be outside the source tree")
            output.parent.mkdir(parents=True, exist_ok=True)
            output.write_text(json.dumps(rendered, indent=2) + "\n", encoding="utf-8")
        print("delta-evidence: runtime record is valid")
        return 0
    except (OSError, RuntimeError, ValueError, KeyError, TypeError, json.JSONDecodeError) as error:
        print(f"delta-evidence: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
