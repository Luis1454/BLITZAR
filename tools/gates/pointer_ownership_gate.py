"""Enforce raw-pointer and ownership boundaries for the BLITZAR tree."""

from __future__ import annotations

import argparse
import dataclasses
import json
import pathlib
import re
import sys

from tools.gates.argument_gate import strip_comments_and_literals


POINTER_BASE = (
    r"(?:auto|void|char|signed\s+char|unsigned\s+char|short|unsigned\s+short|"
    r"int|unsigned\s+int|long|unsigned\s+long|float|double|"
    r"std::[A-Za-z_]\w*(?:::[A-Za-z_]\w*)*|"
    r"blitzar_[A-Za-z_]\w*(?:::[A-Za-z_]\w*)*|"
    r"hip[A-Za-z_]\w*|"
    r"(?:[A-Za-z_]\w*::)+[A-Z][A-Za-z0-9_]*|"
    r"[A-Z][A-Za-z0-9_]*(?:::[A-Za-z_]\w*)*)"
)
RAW_POINTER_RE = re.compile(
    rf"(?:(?P<line_start>^\s*(?:extern\s+\"C\"\s+|BLITZAR_API\s+)?|"
    r"[,(]\s*))(?P<type>(?:const\s+|volatile\s+)*"
    rf"{POINTER_BASE})\s*(?P<stars>\*{{1,2}})\s*(?P<name>[A-Za-z_]\w*)",
    re.MULTILINE,
)
RAW_ALLOCATION_RE = re.compile(r"\b(?:new|delete)\s+(?!\()")
OWNER_NAME_RE = re.compile(r"(?:owner|owned|allocation|resource|storage|impl)", re.IGNORECASE)
SOURCE_SUFFIXES = {".c", ".cc", ".cpp", ".cu", ".h", ".hip", ".hpp", ".inl"}


@dataclasses.dataclass(frozen=True)
class Finding:
    path: str
    line: int
    name: str
    kind: str
    category: str
    ownership: str


@dataclasses.dataclass(frozen=True)
class Violation:
    path: str
    line: int
    message: str


def load_contract(root: pathlib.Path) -> dict[str, object]:
    path = root / "plan" / "pointer_ownership.json"
    return json.loads(path.read_text(encoding="utf-8"))


def relative_path(root: pathlib.Path, path: pathlib.Path) -> str:
    return path.relative_to(root).as_posix()


def source_files(root: pathlib.Path, contract: dict[str, object]) -> list[pathlib.Path]:
    suffixes = {str(item).lower() for item in contract["suffixes"]}
    files: set[pathlib.Path] = set()
    for configured_root in contract["scan_roots"]:
        scan_root = root / str(configured_root)
        if not scan_root.is_dir():
            continue
        files.update(
            path
            for path in scan_root.rglob("*")
            if path.is_file() and path.suffix.lower() in suffixes
        )
    return sorted(files)


def line_number(source: str, offset: int) -> int:
    return source.count("\n", 0, offset) + 1


def allowlist(contract: dict[str, object]) -> dict[str, dict[str, str]]:
    entries = contract.get("allowlist", [])
    return {
        str(entry["path"]): {
            "category": str(entry["category"]),
            "ownership": str(entry["ownership"]),
        }
        for entry in entries
        if isinstance(entry, dict)
    }


def pointer_findings(
    root: pathlib.Path,
    path: pathlib.Path,
    registered: dict[str, dict[str, str]],
    allocation_paths: set[str],
) -> tuple[list[Finding], list[Violation]]:
    relative = relative_path(root, path)
    source = path.read_text(encoding="utf-8")
    clean = strip_comments_and_literals(source)
    specification = registered.get(relative)
    findings: list[Finding] = []
    violations: list[Violation] = []

    for match in RAW_POINTER_RE.finditer(clean):
        if specification is None:
            violations.append(
                Violation(relative, line_number(clean, match.start()),
                          "raw pointer is not registered at an ABI or execution boundary")
            )
            continue

        finding = Finding(
            relative,
            line_number(clean, match.start()),
            match.group("name"),
            "raw_pointer",
            specification["category"],
            specification["ownership"],
        )
        findings.append(finding)
        if specification["category"] == "device_view" and OWNER_NAME_RE.search(finding.name):
            violations.append(
                Violation(relative, finding.line,
                          "device view uses an owner-like raw pointer name")
            )

    for match in RAW_ALLOCATION_RE.finditer(clean):
        line = line_number(clean, match.start())
        if relative not in allocation_paths:
            violations.append(
                Violation(relative, line, "direct new/delete is outside the C ABI handle boundary")
            )
        findings.append(
            Finding(relative, line, match.group(0).strip(), "raw_allocation", "allocation_boundary", "handle_boundary")
        )

    return findings, violations


def build_report(root: pathlib.Path) -> dict[str, object]:
    contract = load_contract(root)
    registered = allowlist(contract)
    allocation_paths = {str(item) for item in contract.get("allocation_allowlist", [])}
    files = source_files(root, contract)
    findings: list[Finding] = []
    violations: list[Violation] = []

    for configured_path in registered:
        if not (root / pathlib.PurePosixPath(configured_path)).is_file():
            violations.append(Violation(configured_path, 0, "registered boundary file is missing"))

    for path in files:
        file_findings, file_violations = pointer_findings(
            root, path, registered, allocation_paths
        )
        findings.extend(file_findings)
        violations.extend(file_violations)

    return {
        "schema_version": 1,
        "scanned_files": len(files),
        "registered_boundaries": len(registered),
        "findings": [dataclasses.asdict(item) for item in findings],
        "violations": [dataclasses.asdict(item) for item in violations],
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=pathlib.Path, default=pathlib.Path(__file__).resolve().parents[2])
    parser.add_argument("--output", type=pathlib.Path)
    parser.add_argument("--check", action="store_true")
    arguments = parser.parse_args(argv)
    root = arguments.root.resolve()

    try:
        report = build_report(root)
    except (OSError, ValueError, json.JSONDecodeError, TypeError, KeyError) as error:
        print(f"pointer-ownership-gate: {error}", file=sys.stderr)
        return 1

    if arguments.output is not None:
        arguments.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    elif not arguments.check:
        print(json.dumps(report, indent=2))

    violations = report["violations"]
    if arguments.check and violations:
        for violation in violations:
            print(
                f"pointer-ownership-gate: {violation['path']}:{violation['line']}: "
                f"{violation['message']}",
                file=sys.stderr,
            )
        return 1

    print(
        f"pointer-ownership-gate: {report['scanned_files']} source files, "
        f"{len(report['findings'])} registered findings, {len(violations)} violations",
        file=sys.stderr,
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
