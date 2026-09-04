"""Run and publish the external CPU Grid, PM, and TreePM evidence matrix."""

from __future__ import annotations

import argparse
import json
import pathlib
import platform
import subprocess
import sys
from typing import Any

from tools.evidence.pm_contract import load_contract, validate_contract


TARGETS = ["blitzar_pm_grid_test", "blitzar_pm_test", "blitzar_treepm_test"]


def executable_path(build_dir: pathlib.Path, target: str) -> pathlib.Path | None:
    candidates = [build_dir / target]
    if sys.platform == "win32":
        candidates.insert(0, build_dir / f"{target}.exe")
    return next((candidate for candidate in candidates if candidate.is_file()), None)


def external_output(root: pathlib.Path, output: pathlib.Path) -> bool:
    return output != root and root not in output.parents


def repository_revision(root: pathlib.Path) -> str | None:
    result = subprocess.run(
        ["git", "-C", str(root), "rev-parse", "HEAD"],
        check=False,
        capture_output=True,
        text=True,
    )
    return result.stdout.strip() if result.returncode == 0 else None


def run_targets(build_dir: pathlib.Path, root: pathlib.Path) -> tuple[list[dict[str, Any]], list[str]]:
    records: list[dict[str, Any]] = []
    errors: list[str] = []
    for target in TARGETS:
        executable = executable_path(build_dir, target)
        if executable is None:
            errors.append(f"missing target: {target}")
            continue
        process = subprocess.run(
            [str(executable)], cwd=root, check=False, capture_output=True, text=True, timeout=120
        )
        records.append(
            {
                "target": target,
                "returncode": process.returncode,
                "stdout": process.stdout,
                "stderr": process.stderr,
            }
        )
        if process.returncode != 0:
            errors.append(f"target returned {process.returncode}: {target}")
    return records, errors


def write_artifacts(
    output: pathlib.Path,
    root: pathlib.Path,
    contract: dict[str, Any],
    records: list[dict[str, Any]],
    errors: list[str],
) -> None:
    output.mkdir(parents=True, exist_ok=True)
    (output / "metadata.json").write_text(
        json.dumps(
            {
                "revision": repository_revision(root),
                "platform": {"system": platform.system(), "machine": platform.machine()},
                "contract": {"plan_version": contract["plan_version"], "phase": contract["phase"]},
                "record_count": len(records),
                "errors": errors,
            },
            indent=2,
            sort_keys=True,
        )
        + "\n",
        encoding="utf-8",
    )
    (output / "results.json").write_text(
        json.dumps(records, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    status = "passed" if not errors else "failed"
    summary = [
        "# BLITZAR Grid PM Evidence",
        "",
        f"- status: `{status}`",
        f"- plan version: `{contract['plan_version']}`",
        f"- targets: `{len(records)}`",
        "- scope: deterministic single-rank CPU correctness and lifecycle",
        "- timing and distributed mesh claims: not made",
        "",
    ]
    summary.extend(f"- {error}" for error in errors)
    (output / "summary.md").write_text("\n".join(summary) + "\n", encoding="utf-8")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=pathlib.Path, required=True)
    parser.add_argument("--build-dir", type=pathlib.Path)
    parser.add_argument("--output", type=pathlib.Path)
    parser.add_argument("--check", action="store_true")
    arguments = parser.parse_args(argv)
    root = arguments.root.resolve()
    try:
        contract = load_contract(root)
        contract_errors = validate_contract(contract)
        if arguments.check:
            if contract_errors:
                for error in contract_errors:
                    print(f"pm-evidence: {error}", file=sys.stderr)
                return 1
            print(f"pm-evidence: contract {contract['plan_version']} is valid")
            return 0
        if arguments.build_dir is None or arguments.output is None:
            parser.error("--build-dir and --output are required unless --check is used")
        output = arguments.output.resolve()
        if not external_output(root, output):
            raise ValueError("PM evidence output must be outside the source tree")
        records, process_errors = run_targets(arguments.build_dir.resolve(), root)
        errors = contract_errors + process_errors
        write_artifacts(output, root, contract, records, errors)
        if errors:
            for error in errors:
                print(f"pm-evidence: {error}", file=sys.stderr)
            return 1
        print(f"pm-evidence: {len(records)} targets passed")
        return 0
    except (OSError, ValueError, json.JSONDecodeError, subprocess.TimeoutExpired) as error:
        print(f"pm-evidence: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
