"""Enforce the single production boundary for native MPI symbols and headers."""

from __future__ import annotations

import argparse
import dataclasses
import json
import pathlib
import re
import sys


FORBIDDEN_MPI = re.compile(
    r"#\s*include\s*[<\"]mpi\.h[>\"]|\bMPI_[A-Za-z0-9_]+\b|\bBLITZAR_HAS_MPI\b"
)
DEFAULT_SUFFIXES = {".c", ".cc", ".cpp", ".cu", ".h", ".hip", ".hpp", ".inl"}


@dataclasses.dataclass(frozen=True)
class Violation:
    path: str
    line: int
    message: str


def load_boundary(root: pathlib.Path) -> dict[str, object]:
    quality_path = root / "plan" / "quality.json"
    quality = json.loads(quality_path.read_text(encoding="utf-8"))
    boundary = quality.get("mpi_boundary")

    if not isinstance(boundary, dict):
        raise ValueError("quality manifest needs an mpi_boundary contract")

    for key in ("native_units", "test_native_units", "scan_roots", "suffixes"):
        values = boundary.get(key)
        if not isinstance(values, list) or not all(isinstance(item, str) and item for item in values):
            raise ValueError(f"mpi_boundary.{key} is invalid")

    return boundary


def source_files(root: pathlib.Path, boundary: dict[str, object]) -> list[pathlib.Path]:
    suffixes = {str(item).lower() for item in boundary["suffixes"]}
    files: list[pathlib.Path] = []

    for configured_root in boundary["scan_roots"]:
        scan_root = root / str(configured_root)
        if not scan_root.is_dir():
            continue
        files.extend(path for path in scan_root.rglob("*") if path.is_file() and path.suffix in suffixes)

    return sorted(files)


def relative_path(root: pathlib.Path, path: pathlib.Path) -> str:
    return path.relative_to(root).as_posix()


def scan_file(root: pathlib.Path, path: pathlib.Path, allowed: set[str]) -> list[Violation]:
    relative = relative_path(root, path)
    if relative in allowed:
        return []

    violations: list[Violation] = []
    for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
        if FORBIDDEN_MPI.search(line) is not None:
            violations.append(
                Violation(relative, line_number, "MPI header, handle, or build symbol escaped the native boundary")
            )

    return violations


def build_report(root: pathlib.Path) -> dict[str, object]:
    boundary = load_boundary(root)
    native_units = [str(item) for item in boundary["native_units"]]
    test_native_units = [str(item) for item in boundary["test_native_units"]]
    allowed = set(native_units + test_native_units)
    files = source_files(root, boundary)
    violations = [item for path in files for item in scan_file(root, path, allowed)]

    for configured in native_units:
        if not (root / pathlib.PurePosixPath(configured)).is_file():
            violations.append(Violation(configured, 0, "registered native unit is missing"))

    native_cpp = [item for item in native_units if item.endswith(".cpp")]
    native_headers = [item for item in native_units if item.endswith((".h", ".hpp"))]
    if not native_cpp or not native_headers:
        violations.append(Violation("plan/quality.json", 0, "native boundary needs implementation and state units"))

    return {
        "schema_version": 1,
        "native_units": native_units,
        "test_native_units": test_native_units,
        "scanned_files": len(files),
        "violations": [dataclasses.asdict(item) for item in violations],
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=pathlib.Path, default=pathlib.Path(__file__).resolve().parents[1])
    parser.add_argument("--output", type=pathlib.Path)
    parser.add_argument("--check", action="store_true")
    arguments = parser.parse_args(argv)
    root = arguments.root.resolve()

    try:
        report = build_report(root)
    except (OSError, ValueError, json.JSONDecodeError, TypeError, KeyError) as error:
        print(f"mpi-boundary-gate: {error}", file=sys.stderr)
        return 1

    if arguments.output is not None:
        arguments.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    elif not arguments.check:
        print(json.dumps(report, indent=2))

    violations = report["violations"]
    if arguments.check and violations:
        for violation in violations:
            location = f"{violation['path']}:{violation['line']}"
            print(f"mpi-boundary-gate: {location}: {violation['message']}", file=sys.stderr)
        return 1

    print(
        f"mpi-boundary-gate: {report['scanned_files']} source files, "
        f"{len(violations)} violations",
        file=sys.stderr,
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
