"""Validate the frozen clean-room plan and its executable test contract."""

from __future__ import annotations

import json
import pathlib
import re
import sys


ROOT = pathlib.Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "plan" / "manifest.json"
QUALITY = ROOT / "plan" / "quality.json"
CMESSAGE = ROOT / "CMakeLists.txt"

TEST_ID_PATTERN = re.compile(r"^TST-[A-Z0-9]+(?:-[A-Z0-9]+)+$")
TEST_PATTERN = re.compile(
    r"add_test\s*\(\s*NAME\s+([^\s\)]+)\s+COMMAND\s+([^\)]*)\)",
    re.MULTILINE,
)
SOURCE_SUFFIXES = {
    ".c",
    ".cpp",
    ".cu",
    ".cuh",
    ".hip",
    ".h",
    ".hpp",
    ".inl",
}
PUBLIC_HEADER_NAMES = {"blitzar.h", "blitzar.hpp"}


def fail(message: str) -> None:
    print(f"plan-check: {message}", file=sys.stderr)
    raise SystemExit(1)


def load_json(path: pathlib.Path, label: str) -> dict:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        fail(f"{label} is not valid JSON: {error}")
    if not isinstance(value, dict):
        fail(f"{label} must contain a JSON object")
    return value


def normalize_manifest_path(value: object, label: str) -> pathlib.PurePosixPath:
    if not isinstance(value, str) or not value:
        fail(f"{label} must contain non-empty relative paths")
    path = pathlib.PurePosixPath(value)
    if path.is_absolute() or "" in path.parts or ".." in path.parts:
        fail(f"{label} contains unsafe path: {value}")
    if "\\" in value:
        fail(f"{label} must use forward slashes: {value}")
    return path


def validate_roots(data: dict) -> None:
    roots = data.get("roots")
    deferred = data.get("deferred_roots", [])
    if not isinstance(roots, list) or not roots:
        fail("roots must be a non-empty list")
    if not isinstance(deferred, list):
        fail("deferred_roots must be a list")

    root_paths = [normalize_manifest_path(root, "roots") for root in roots]
    deferred_paths = [
        normalize_manifest_path(root, "deferred_roots") for root in deferred
    ]
    all_paths = [path.as_posix().casefold() for path in root_paths + deferred_paths]
    if len(all_paths) != len(set(all_paths)):
        fail("roots and deferred_roots must be unique")

    for root in root_paths:
        directory = ROOT.joinpath(*root.parts)
        if not directory.is_dir():
            fail(f"materialized root is missing: {root}")
        if not any(directory.iterdir()):
            fail(f"materialized root is empty: {root}")

    for root in deferred_paths:
        directory = ROOT.joinpath(*root.parts)
        if directory.exists():
            fail(f"deferred root is materialized; promote it to roots: {root}")


def validate_phases(data: dict) -> set[str]:
    phases = data.get("phases")
    if not isinstance(phases, list) or not phases:
        fail("at least one phase is required")
    phase_ids = [phase.get("id") for phase in phases]
    expected_ids = [f"P{index}" for index in range(len(phases))]
    if phase_ids != expected_ids:
        fail("phase identifiers must be contiguous and ordered")
    if any(not phase.get("name") for phase in phases):
        fail("every phase needs a name")
    return set(phase_ids)


def validate_forbidden_references(data: dict) -> None:
    forbidden = data.get("forbidden_references")
    if not isinstance(forbidden, list) or not forbidden:
        fail("forbidden_references must be declared")
    code_roots = [
        ROOT / "src",
        ROOT / "include",
        ROOT / "apps",
        ROOT / "tests",
        ROOT / "examples",
    ]
    for directory in code_roots:
        if not directory.is_dir():
            continue
        for path in directory.rglob("*"):
            if not path.is_file():
                continue
            text = path.read_text(encoding="utf-8", errors="ignore").lower()
            for reference in forbidden:
                if str(reference).lower() in text:
                    fail(
                        f"{path.relative_to(ROOT)} contains forbidden reference: {reference}"
                    )


def validate_quality_tests(phase_ids: set[str]) -> None:
    quality = load_json(QUALITY, "quality manifest")
    tests = quality.get("tests")
    if not isinstance(tests, list) or not tests:
        fail("quality manifest needs named tests with IDs")

    commands: dict[str, str] = {}
    ids: set[str] = set()
    for test in tests:
        if not isinstance(test, dict):
            fail("every quality test must be an object")
        test_id = test.get("id")
        name = test.get("name")
        command = test.get("command")
        phase = test.get("phase")
        if (
            not isinstance(test_id, str)
            or not TEST_ID_PATTERN.fullmatch(test_id)
            or test_id in ids
        ):
            fail(f"invalid or duplicate quality test ID: {test_id}")
        if not isinstance(name, str) or not name:
            fail(f"quality test {test_id} needs a name")
        if (
            not isinstance(command, str)
            or not command
            or command in commands.values()
        ):
            fail(f"quality test {test_id} needs a unique command")
        if phase not in phase_ids:
            fail(f"quality test {test_id} references unknown phase: {phase}")
        ids.add(test_id)
        commands[test_id] = command

    def normalize_cmake_command(command: str) -> str:
        normalized = command
        normalized = normalized.replace("${MPIEXEC_EXECUTABLE}", "mpiexec")
        normalized = normalized.replace("${MPIEXEC_NUMPROC_FLAG}", "-np")
        normalized = normalized.replace("${MPIEXEC_PREFLAGS}", "")
        normalized = normalized.replace("${MPIEXEC_POSTFLAGS}", "")
        normalized = re.sub(
            r"\$<TARGET_FILE:([^>]+)>",
            r"\1",
            normalized,
        )
        return " ".join(normalized.split())

    cmake_tests = {
        test_id: normalize_cmake_command(command)
        for test_id, command in TEST_PATTERN.findall(
            CMESSAGE.read_text(encoding="utf-8")
        )
    }
    if set(cmake_tests) != ids:
        missing = sorted(ids - set(cmake_tests))
        extra = sorted(set(cmake_tests) - ids)
        fail(f"CTest manifest mismatch; missing={missing}, extra={extra}")
    for test_id, command in commands.items():
        if cmake_tests[test_id] != command:
            fail(
                f"CTest command mismatch for {test_id}: "
                f"manifest={command}, CMake={cmake_tests[test_id]}"
            )


def validate_naming() -> None:
    code_roots = [
        ROOT / "src",
        ROOT / "include",
        ROOT / "apps",
        ROOT / "tests",
        ROOT / "examples",
    ]
    files = [
        path
        for directory in code_roots
        if directory.is_dir()
        for path in directory.rglob("*")
        if path.is_file() and path.suffix.lower() in SOURCE_SUFFIXES
    ]
    names: dict[str, pathlib.Path] = {}
    for path in files:
        key = path.name.casefold()
        if key in names:
            fail(
                f"duplicate source filename: {path.relative_to(ROOT)} and "
                f"{names[key].relative_to(ROOT)}"
            )
        names[key] = path
        if path.name in PUBLIC_HEADER_NAMES:
            continue
        if path.suffix.lower() in {
            ".cpp",
            ".cu",
            ".cuh",
            ".hip",
            ".hpp",
            ".inl",
        }:
            if re.fullmatch(
                r"[A-Z][A-Za-z0-9]*\.(cpp|cu|cuh|hip|hpp|inl)", path.name
            ) is None:
                fail(f"non-PascalCase C++/CUDA filename: {path.relative_to(ROOT)}")
        elif path.suffix.lower() in {".c", ".h"}:
            if re.fullmatch(r"[A-Z][A-Za-z0-9]*\.(c|h)", path.name) is None:
                fail(f"non-PascalCase C filename: {path.relative_to(ROOT)}")


def main() -> None:
    if not (ROOT / "PLAN.md").is_file():
        fail("PLAN.md is missing")
    if not MANIFEST.is_file():
        fail("plan/manifest.json is missing")
    if not QUALITY.is_file():
        fail("plan/quality.json is missing")
    if not CMESSAGE.is_file():
        fail("CMakeLists.txt is missing")

    manifest = load_json(MANIFEST, "manifest")
    if manifest.get("status") != "frozen":
        fail("manifest status must remain 'frozen'")
    if not manifest.get("plan_version"):
        fail("plan_version is required")
    validate_roots(manifest)
    phase_ids = validate_phases(manifest)
    validate_forbidden_references(manifest)
    validate_quality_tests(phase_ids)
    validate_naming()
    print(f"plan-check: frozen plan {manifest['plan_version']} is valid")


if __name__ == "__main__":
    main()
