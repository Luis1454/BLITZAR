"""Build the final deterministic repository qualification report."""

from __future__ import annotations

import argparse
import json
import pathlib
import sys

from tools.audit.final_audit_contract import is_inside, load_json, validate_contract
from tools.audit.final_audit_report import build_audit, write_artifacts
from tools.audit.final_audit_scan import audit_files


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--root", type=pathlib.Path, default=pathlib.Path(__file__).resolve().parents[2]
    )
    parser.add_argument("--output", type=pathlib.Path)
    parser.add_argument("--strict", action="store_true")
    parser.add_argument("--run-gates", action="store_true")
    parser.add_argument("--check", action="store_true")
    arguments = parser.parse_args(argv)
    root = arguments.root.resolve()
    try:
        contract = load_json(root / "plan" / "final_audit.json")
        contract_errors = validate_contract(root, contract)
        if arguments.check:
            _, ownership_errors = audit_files(root, contract)
            errors = contract_errors + ownership_errors
            if errors:
                for error in errors:
                    print(f"final-audit: {error}", file=sys.stderr)
                return 1
            print("final-audit: contract and tracked-file ownership are valid")
            return 0
        if arguments.output is None:
            parser.error("--output is required unless --check is used")
        output = arguments.output.resolve()
        if is_inside(output, root):
            raise ValueError("final audit output must be outside the source tree")
        report, gate = build_audit(root, contract, arguments.run_gates, arguments.strict)
        write_artifacts(output, report, gate)
        for error in report["errors"]:
            print(f"final-audit: {error}", file=sys.stderr)
        print(f"final-audit: output={output}")
        print(f"final-audit: files={report['file_count']}")
        print(f"final-audit: status={report['status']}")
        print(f"final-audit: unresolved={len(report['findings']['unresolved'])}")
        return int(bool(report["errors"]))
    except (OSError, RuntimeError, ValueError, KeyError, TypeError, json.JSONDecodeError) as error:
        print(f"final-audit: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
