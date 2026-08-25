"""Validate test responsibility mapping and release-safe test conventions."""

from __future__ import annotations

import argparse
import json
import pathlib
import re
import sys

from architecture_metrics import scan_file_functions
from argument_gate import strip_comments_and_literals
from architecture_sources import configured_paths, load_source_completeness


TEST_SOURCE_SUFFIXES = {".c", ".cc", ".cpp", ".cu", ".hip", ".h", ".hpp", ".inl"}
TEST_ID_PATTERN = re.compile(r"\bTST-[A-Z0-9]+(?:-[A-Z0-9]+)+\b")
ASSERT_PATTERN = re.compile(r"\bassert\s*\(")


def load_json(path: pathlib.Path) -> object:
    return json.loads(path.read_text(encoding="utf-8"))


def quality_tests(root: pathlib.Path) -> dict[str, dict[str, object]]:
    quality = load_json(root / "plan" / "quality.json")
    tests = quality.get("tests") if isinstance(quality, dict) else None
    if not isinstance(tests, list):
        raise ValueError("quality manifest needs a tests list")

    result: dict[str, dict[str, object]] = {}
    for item in tests:
        if not isinstance(item, dict) or not isinstance(item.get("id"), str):
            raise ValueError("quality test entries need an id")
        test_id = str(item["id"])
        if test_id in result:
            raise ValueError(f"duplicate quality test id: {test_id}")
        result[test_id] = item
    return result


def cmake_test_ids(root: pathlib.Path) -> set[str]:
    configured = load_source_completeness(root)
    source = "\n".join(
        path.read_text(encoding="utf-8")
        for path in configured_paths(root, configured["cmake_files"])
    )
    return set(TEST_ID_PATTERN.findall(source))


def load_cases(root: pathlib.Path) -> list[dict[str, object]]:
    data = load_json(root / "plan" / "test_map.json")
    cases = data.get("cases") if isinstance(data, dict) else None
    if not isinstance(cases, list):
        raise ValueError("test map needs a cases list")
    if not all(isinstance(item, dict) for item in cases):
        raise ValueError("test map cases must be objects")
    return [item for item in cases if isinstance(item, dict)]


def validate_case_files(root: pathlib.Path, test_id: str, case: dict[str, object]) -> list[str]:
    errors: list[str] = []
    for field in ("entrypoint", "responsibility"):
        value = case.get(field)
        if not isinstance(value, str) or not value:
            errors.append(f"{test_id}: missing {field}")
            continue
        if not (root / value).is_file():
            errors.append(f"{test_id}: missing mapped file {value}")

    if not isinstance(case.get("contract"), str) or not case["contract"]:
        errors.append(f"{test_id}: missing contract")
    return errors


def validate_case_command(
    test_id: str, case: dict[str, object], quality: dict[str, dict[str, object]]
) -> list[str]:
    errors: list[str] = []
    mode = case.get("mode")
    ranks = case.get("ranks")
    if not isinstance(mode, str) or not isinstance(ranks, int) or ranks < 1:
        errors.append(f"{test_id}: mode and positive ranks are required")

    quality_case = quality.get(test_id)
    if quality_case is None:
        errors.append(f"{test_id}: missing from quality manifest")
    elif isinstance(quality_case.get("command"), str):
        command = str(quality_case["command"])
        if "blitzar_mpi_test" in command and mode not in command and mode != "":
            errors.append(f"{test_id}: mapped mode is absent from CTest command")
    return errors


def validate_mapping(root: pathlib.Path) -> list[str]:
    quality = quality_tests(root)
    cases = load_cases(root)
    errors: list[str] = []
    mapped: dict[str, dict[str, object]] = {}

    for case in cases:
        test_id = case.get("id")
        if not isinstance(test_id, str):
            errors.append("test map case is missing a string id")
            continue
        if test_id in mapped:
            errors.append(f"duplicate test map id: {test_id}")
        mapped[test_id] = case
        errors.extend(validate_case_files(root, test_id, case))
        errors.extend(validate_case_command(test_id, case, quality))

    missing = sorted(set(quality) - set(mapped))
    extra = sorted(set(mapped) - set(quality))
    errors.extend(f"missing test map entry: {test_id}" for test_id in missing)
    errors.extend(f"test map id is not in quality manifest: {test_id}" for test_id in extra)

    cmake_ids = cmake_test_ids(root)
    errors.extend(f"missing CTest registration: {test_id}" for test_id in sorted(set(quality) - cmake_ids))
    errors.extend(f"unmapped CTest registration: {test_id}" for test_id in sorted(cmake_ids - set(quality)))

    return errors


def validate_assert_free_tests(root: pathlib.Path) -> list[str]:
    errors: list[str] = []
    for path in sorted((root / "tests").rglob("*")):
        if not path.is_file() or path.suffix.lower() not in TEST_SOURCE_SUFFIXES:
            continue
        source = path.read_text(encoding="utf-8", errors="replace")
        if ASSERT_PATTERN.search(strip_comments_and_literals(source)):
            errors.append(f"assert is forbidden in test source: {path.relative_to(root).as_posix()}")
    return errors


def architecture_signals(functions: tuple[object, ...], thresholds: dict[str, int]) -> set[str]:
    signals: set[str] = set()
    if len(functions) > thresholds["max_functions_per_file"]:
        signals.add("function_count")
    if any(item.body_lines > thresholds["max_function_lines"] for item in functions):
        signals.add("function_length")
    if any(item.branch_points > thresholds["max_branch_points"] for item in functions):
        signals.add("branching")
    if any(item.parameters > thresholds["max_parameters"] for item in functions):
        signals.add("parameter_count")
    return signals


def validate_test_architecture(root: pathlib.Path) -> list[str]:
    quality = load_json(root / "plan" / "quality.json")
    architecture = quality.get("architecture", {})
    thresholds = architecture.get("thresholds", {})
    reviews_data = load_json(root / "plan" / "architecture_reviews.json")
    reviews = {
        item.get("path"): set(item.get("signals", []))
        for item in reviews_data.get("reviews", [])
        if isinstance(item, dict) and isinstance(item.get("path"), str)
    }
    errors: list[str] = []

    for path in sorted((root / "tests").rglob("*")):
        if not path.is_file() or path.suffix.lower() not in TEST_SOURCE_SUFFIXES:
            continue
        relative = path.relative_to(root).as_posix()
        source = path.read_text(encoding="utf-8", errors="replace")
        functions = scan_file_functions(source, relative)
        signals = architecture_signals(functions, thresholds)
        if signals and not signals.issubset(reviews.get(relative, set())):
            errors.append(
                f"{relative}: architecture signals need an exact accepted review "
                f"({sorted(signals)})"
            )

    return errors


def run(root: pathlib.Path) -> list[str]:
    errors = validate_mapping(root)
    errors.extend(validate_assert_free_tests(root))
    errors.extend(validate_test_architecture(root))
    return errors


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=pathlib.Path, default=pathlib.Path(__file__).resolve().parents[1])
    parser.add_argument("--check", action="store_true")
    arguments = parser.parse_args(argv)
    root = arguments.root.resolve()

    try:
        errors = run(root)
    except (OSError, ValueError, json.JSONDecodeError, KeyError, TypeError) as error:
        print(f"test-suite-gate: {error}", file=sys.stderr)
        return 1

    if errors:
        for error in errors:
            print(f"test-suite-gate: {error}", file=sys.stderr)
        return 1

    print(f"test-suite-gate: {len(quality_tests(root))} mapped test cases; checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
