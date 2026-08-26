"""Check the public SDK boundary and registered ABI exceptions."""

from __future__ import annotations

import argparse
import dataclasses
import json
import pathlib
import re
import sys

from tools.gates.argument_gate import load_exceptions, scan_source, strip_comments_and_literals


SOURCE_SUFFIXES = {".c", ".cc", ".cpp", ".h", ".hip", ".hpp", ".inl"}
INCLUDE_RE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]')
RAW_POINTER_RE = re.compile(r"(?<![\w:])[A-Za-z_]\w*(?:::\w+)?\s*\*+")
FORBIDDEN_SYMBOL_RE = re.compile(r"\b(?:MPI_[A-Z_]+|hip[A-Za-z_]*|cuda[A-Za-z_]*)\b")


@dataclasses.dataclass(frozen=True)
class Violation:
    path: str
    line: int
    message: str


def load_contract(root: pathlib.Path) -> dict[str, object]:
    path = root / "plan" / "public_boundary.json"
    return json.loads(path.read_text(encoding="utf-8"))


def relative(root: pathlib.Path, path: pathlib.Path) -> str:
    return path.relative_to(root).as_posix()


def source_files(root: pathlib.Path, directory: pathlib.Path) -> list[pathlib.Path]:
    return sorted(
        path
        for path in directory.rglob("*")
        if path.is_file() and path.suffix.lower() in SOURCE_SUFFIXES
    )


def include_lines(source: str) -> list[tuple[int, str]]:
    return [
        (line_number, match.group(1))
        for line_number, line in enumerate(source.splitlines(), 1)
        if (match := INCLUDE_RE.match(line)) is not None
    ]


def line_number(source: str, offset: int) -> int:
    return source.count("\n", 0, offset) + 1


def scan_public_header(
    root: pathlib.Path,
    path: pathlib.Path,
    specification: dict[str, object],
    forbidden_tokens: list[str],
) -> list[Violation]:
    source = path.read_text(encoding="utf-8")
    clean = strip_comments_and_literals(source)
    relative_path = relative(root, path)
    allowed_includes = set(specification.get("allowed_includes", []))
    violations: list[Violation] = []

    for line, include in include_lines(source):
        if include not in allowed_includes:
            violations.append(Violation(relative_path, line, f"include is not registered: {include}"))
        if any(token.lower() in include.lower() for token in forbidden_tokens):
            violations.append(Violation(relative_path, line, f"forbidden public include: {include}"))

    for token in forbidden_tokens:
        if token in {"MPI_", "hip", "cuda"}:
            match = FORBIDDEN_SYMBOL_RE.search(clean)
            if match is not None:
                violations.append(
                    Violation(relative_path, line_number(clean, match.start()),
                              f"forbidden public symbol: {match.group(0)}")
                )
                break
        elif token in clean:
            violations.append(
                Violation(relative_path, line_number(clean, clean.index(token)),
                          f"forbidden public token: {token}")
            )

    pointer_policy = specification.get("raw_pointer_policy")
    if pointer_policy != "c_abi":
        match = RAW_POINTER_RE.search(clean)
        if match is not None:
            violations.append(
                Violation(relative_path, line_number(clean, match.start()),
                          "raw pointer is forbidden in the C++ public facade")
            )

    if pointer_policy == "c_abi":
        for declaration in specification.get("opaque_handle_declarations", []):
            if declaration not in source:
                violations.append(Violation(relative_path, 1, f"missing opaque declaration: {declaration}"))

    if pointer_policy == "c_abi":
        for forbidden_type in specification.get("fixed_width_forbidden_types", []):
            match = re.search(rf"\b{re.escape(forbidden_type)}\b", clean)
            if match is not None:
                violations.append(
                    Violation(relative_path, line_number(clean, match.start()),
                              f"non-fixed-width ABI type is forbidden: {forbidden_type}")
                )

    return violations


def scan_internal_headers(
    root: pathlib.Path, contract: dict[str, object]
) -> list[Violation]:
    exceptions = {
        (str(item["path"]), str(item["include"]))
        for item in contract.get("internal_cpp_facade_exceptions", [])
    }
    violations: list[Violation] = []
    for path in source_files(root, root / "src"):
        relative_path = relative(root, path)
        source = path.read_text(encoding="utf-8")
        for line, include in include_lines(source):
            if include != "blitzar/blitzar.hpp":
                continue
            if (relative_path, include) not in exceptions:
                violations.append(
                    Violation(relative_path, line, "internal header leaks the public C++ facade")
                )
    return violations


def scan_parameters(root: pathlib.Path, paths: list[pathlib.Path]) -> list[Violation]:
    exceptions = load_exceptions(root)
    violations: list[Violation] = []
    for path in paths:
        relative_path = relative(root, path)
        findings = scan_source(path.read_text(encoding="utf-8"), relative_path, 4)
        for finding in findings:
            if (finding.path, finding.name) not in exceptions:
                violations.append(
                    Violation(finding.path, finding.line,
                              f"public callable has {finding.parameters} parameters: {finding.name}")
                )
    return violations


def validate(root: pathlib.Path) -> list[Violation]:
    contract = load_contract(root)
    public_specifications = contract.get("public_headers", {})
    public_directory = root / "include" / "blitzar"
    declared = {str(path) for path in public_specifications}
    actual_paths = {relative(root, path) for path in source_files(root, public_directory)}
    violations: list[Violation] = []

    for path in sorted(declared - actual_paths):
        violations.append(Violation(path, 1, "declared public header is missing"))
    for path in sorted(actual_paths - declared):
        violations.append(Violation(path, 1, "public header is not registered"))

    forbidden_tokens = [str(token) for token in contract.get("forbidden_public_tokens", [])]
    pointer_exceptions = {
        str(item["path"])
        for item in contract.get("raw_pointer_exceptions", [])
        if isinstance(item, dict) and "path" in item
    }
    public_paths = []
    for path_string, specification in public_specifications.items():
        path = root / pathlib.PurePosixPath(path_string)
        if not path.is_file() or not isinstance(specification, dict):
            continue
        public_paths.append(path)
        if specification.get("raw_pointer_policy") == "c_abi" and path_string not in pointer_exceptions:
            violations.append(Violation(path_string, 1, "C ABI raw-pointer exception is not registered"))
        violations.extend(scan_public_header(root, path, specification, forbidden_tokens))

    violations.extend(scan_internal_headers(root, contract))
    violations.extend(scan_parameters(root, public_paths))
    return sorted(violations, key=lambda item: (item.path, item.line, item.message))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=pathlib.Path,
                        default=pathlib.Path(__file__).resolve().parents[2])
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--report-only", action="store_true")
    arguments = parser.parse_args()
    root = arguments.root.resolve()
    violations = validate(root)
    for violation in violations:
        print(f"{violation.path}:{violation.line}: {violation.message}")
    public_count = len(load_contract(root).get("public_headers", {}))
    print(f"public-api-gate: {public_count} registered public headers, violations={len(violations)}")
    return 1 if violations and not arguments.report_only else 0


if __name__ == "__main__":
    sys.exit(main())
