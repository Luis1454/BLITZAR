"""Run and publish the external Morton and particle-layout evidence matrix."""

from __future__ import annotations

import argparse
import json
import pathlib
import platform
import subprocess
import sys
from dataclasses import dataclass
from typing import Any

from tools.evidence.layout_contract import (
    load_contract,
    parse_layout_record,
    validate_contract,
    validate_records,
)


@dataclass(frozen=True)
class EvidenceRun:
    output: pathlib.Path
    contract: dict[str, Any]
    records: list[dict[str, Any]]
    errors: list[str]
    process: subprocess.CompletedProcess[str]
    root: pathlib.Path


def executable_path(build_dir: pathlib.Path, target: str) -> pathlib.Path | None:
    candidates = [build_dir / target]
    if sys.platform == "win32":
        candidates.insert(0, build_dir / f"{target}.exe")
    return next((candidate for candidate in candidates if candidate.is_file()), None)


def repository_revision(root: pathlib.Path) -> str | None:
    result = subprocess.run(
        ["git", "-C", str(root), "rev-parse", "HEAD"],
        check=False,
        capture_output=True,
        text=True,
    )
    return result.stdout.strip() if result.returncode == 0 else None


def external_output(root: pathlib.Path, output: pathlib.Path) -> bool:
    return output != root and root not in output.parents


def render_summary(
    contract: dict[str, Any], records: list[dict[str, Any]], errors: list[str]
) -> str:
    status = "passed" if not errors else "failed"
    lines = [
        "# BLITZAR Layout Evidence",
        "",
        f"- status: `{status}`",
        f"- plan version: `{contract['plan_version']}`",
        f"- seed: `{contract['seed']}`",
        f"- records: `{len(records)}`",
        "- cache metric: modeled 64-byte line visits, not hardware misses",
        "- SIMD metric: contiguous scan throughput, not a hardware counter",
        "",
    ]
    if errors:
        lines.extend(["## Findings", ""])
        lines.extend(f"- {error}" for error in errors)
    else:
        lines.extend(["All declared ordering and representation combinations passed.", ""])
    return "\n".join(lines)


def write_artifacts(run: EvidenceRun) -> None:
    run.output.mkdir(parents=True, exist_ok=True)
    (run.output / "run.log").write_text(
        f"stdout:\n{run.process.stdout}\nstderr:\n{run.process.stderr}\n", encoding="utf-8"
    )
    metadata = {
        "revision": repository_revision(run.root),
        "platform": {"system": platform.system(), "machine": platform.machine()},
        "contract": {
            "plan_version": run.contract["plan_version"],
            "seed": run.contract["seed"],
        },
        "record_count": len(run.records),
        "errors": run.errors,
    }
    (run.output / "metadata.json").write_text(
        json.dumps(metadata, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    (run.output / "results.json").write_text(
        json.dumps(run.records, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    (run.output / "summary.md").write_text(
        render_summary(run.contract, run.records, run.errors), encoding="utf-8"
    )


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=pathlib.Path, required=True)
    parser.add_argument("--build-dir", type=pathlib.Path)
    parser.add_argument("--output", type=pathlib.Path)
    parser.add_argument("--strict", action="store_true")
    parser.add_argument("--check", action="store_true")
    arguments = parser.parse_args(argv)
    root = arguments.root.resolve()
    try:
        contract = load_contract(root)
        contract_errors = validate_contract(contract)
        if arguments.check:
            if contract_errors:
                for error in contract_errors:
                    print(f"layout-evidence: {error}", file=sys.stderr)
                return 1
            print(f"layout-evidence: contract {contract['plan_version']} is valid")
            return 0
        if arguments.build_dir is None or arguments.output is None:
            parser.error("--build-dir and --output are required unless --check is used")
        build_dir = arguments.build_dir.resolve()
        output = arguments.output.resolve()
        if not external_output(root, output):
            raise ValueError("layout evidence output must be outside the source tree")
        executable = executable_path(build_dir, contract["target"])
        if executable is None:
            raise FileNotFoundError(f"missing target: {contract['target']}")
        process = subprocess.run(
            [str(executable)], cwd=root, check=False, capture_output=True, text=True
        )
        records: list[dict[str, Any]] = []
        parse_errors: list[str] = []
        for line in process.stdout.splitlines():
            try:
                record = parse_layout_record(line)
            except (TypeError, ValueError) as error:
                parse_errors.append(str(error))
                continue
            if record is not None:
                records.append(record)
        errors = contract_errors + parse_errors + validate_records(records, contract)
        if process.returncode != 0:
            errors.append(f"target returned {process.returncode}")
        write_artifacts(EvidenceRun(output, contract, records, errors, process, root))
        if errors:
            for error in errors:
                print(f"layout-evidence: {error}", file=sys.stderr)
            return 1
        print(f"layout-evidence: {len(records)} records passed")
        return 0
    except (OSError, ValueError, json.JSONDecodeError) as error:
        print(f"layout-evidence: {error}", file=sys.stderr)
        return 1 if arguments.strict or arguments.check else 1


if __name__ == "__main__":
    sys.exit(main())
