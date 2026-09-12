"""Validate declared source and test responsibility-directory symmetry."""

from __future__ import annotations

import argparse
import json
import pathlib
import sys


def load_policy(root: pathlib.Path) -> dict[str, object]:
    quality_path = root / "plan" / "quality.json"
    quality = json.loads(quality_path.read_text(encoding="utf-8"))
    policy = quality.get("directory_symmetry")
    if not isinstance(policy, dict):
        raise ValueError("quality manifest needs a directory_symmetry policy")
    return policy


def normalized_path(value: object, label: str, root_name: str) -> pathlib.PurePosixPath:
    if not isinstance(value, str) or not value:
        raise ValueError(f"{label} must be a non-empty relative path")
    if "\\" in value:
        raise ValueError(f"{label} must use forward slashes: {value}")
    path = pathlib.PurePosixPath(value)
    if path.is_absolute() or any(part in {"", ".", ".."} for part in value.split("/")):
        raise ValueError(f"{label} contains an unsafe path: {value}")
    if not path.parts or path.parts[0] != root_name:
        raise ValueError(f"{label} must be below {root_name}/: {value}")
    return path


def policy_pairs(
    policy: dict[str, object],
) -> tuple[tuple[pathlib.PurePosixPath, pathlib.PurePosixPath], ...]:
    configured = policy.get("pairs")
    if not isinstance(configured, list) or not configured:
        raise ValueError("directory_symmetry needs at least one source/test pair")

    pairs: list[tuple[pathlib.PurePosixPath, pathlib.PurePosixPath]] = []
    seen: set[tuple[str, str]] = set()
    for index, item in enumerate(configured):
        if not isinstance(item, dict):
            raise ValueError(f"directory_symmetry pair {index} must be an object")
        source = normalized_path(item.get("source"), f"pair {index} source", "src")
        tests = normalized_path(item.get("tests"), f"pair {index} tests", "tests")
        key = (source.as_posix(), tests.as_posix())
        if key in seen:
            raise ValueError(f"duplicate directory_symmetry pair: {source} -> {tests}")
        seen.add(key)
        pairs.append((source, tests))
    return tuple(pairs)


def allowed_missing_paths(
    policy: dict[str, object],
    pairs: tuple[tuple[pathlib.PurePosixPath, pathlib.PurePosixPath], ...],
) -> set[tuple[pathlib.PurePosixPath, pathlib.PurePosixPath]]:
    configured = policy.get("allowed_missing", [])
    if not isinstance(configured, list):
        raise ValueError("directory_symmetry allowed_missing must be a list")

    allowed: set[tuple[pathlib.PurePosixPath, pathlib.PurePosixPath]] = set()
    for index, item in enumerate(configured):
        if not isinstance(item, dict):
            raise ValueError(f"allowed_missing entry {index} must be an object")
        source = normalized_path(item.get("source"), f"allowed_missing {index} source", "src")
        tests = normalized_path(item.get("tests"), f"allowed_missing {index} tests", "tests")
        reason = item.get("reason")
        if not isinstance(reason, str) or not reason.strip():
            raise ValueError(f"allowed_missing entry {index} needs a reason")

        matched = False
        for source_root, tests_root in pairs:
            if source_root in source.parents and tests_root in tests.parents:
                if source.relative_to(source_root) != tests.relative_to(tests_root):
                    raise ValueError(
                        f"allowed_missing entry {index} must preserve the relative path"
                    )
                matched = True
                break
        if not matched:
            raise ValueError(
                f"allowed_missing entry is outside a declared pair: {source} -> {tests}"
            )
        key = (source, tests)
        if key in allowed:
            raise ValueError(f"duplicate allowed_missing entry: {source} -> {tests}")
        allowed.add(key)
    return allowed


def directories(root: pathlib.Path) -> set[pathlib.PurePosixPath]:
    result = {pathlib.PurePosixPath(".")}
    result.update(
        pathlib.PurePosixPath(path.relative_to(root).as_posix())
        for path in root.rglob("*")
        if path.is_dir()
    )
    return result


def validate(root: pathlib.Path) -> list[str]:
    policy = load_policy(root)
    pairs = policy_pairs(policy)
    allowed = allowed_missing_paths(policy, pairs)
    violations: list[str] = []
    consumed: set[tuple[pathlib.PurePosixPath, pathlib.PurePosixPath]] = set()
    for source_relative, tests_relative in pairs:
        source_root = root.joinpath(*source_relative.parts)
        tests_root = root.joinpath(*tests_relative.parts)
        if not source_root.is_dir():
            violations.append(f"source symmetry root is missing: {source_relative}")
            continue
        if not tests_root.is_dir():
            violations.append(f"test symmetry root is missing: {tests_relative}")
            continue

        source_directories = directories(source_root)
        test_directories = directories(tests_root)
        for relative in sorted(source_directories - test_directories):
            source_path = source_relative / relative
            tests_path = tests_relative / relative
            key = (source_path, tests_path)
            if key in allowed:
                consumed.add(key)
            else:
                violations.append(
                    f"missing mirrored test directory: {source_path} -> {tests_path}"
                )
        for relative in sorted(test_directories - source_directories):
            source_path = source_relative / relative
            tests_path = tests_relative / relative
            violations.append(
                f"extra mirrored test directory: {tests_path} -> {source_path}"
            )
    for source_path, tests_path in sorted(allowed - consumed):
        violations.append(
            f"stale allowed missing directory: {source_path} -> {tests_path}"
        )
    return violations


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=pathlib.Path, required=True)
    parser.add_argument("--check", action="store_true")
    arguments = parser.parse_args(argv)
    if not arguments.check:
        parser.error("--check is required")
    root = arguments.root.resolve()
    try:
        violations = validate(root)
    except (OSError, ValueError, json.JSONDecodeError) as error:
        print(f"directory-symmetry-gate: {error}", file=sys.stderr)
        return 1
    for violation in violations:
        print(f"directory-symmetry-gate: {violation}", file=sys.stderr)
    print(f"directory-symmetry-gate: violations={len(violations)}")
    return int(bool(violations))


if __name__ == "__main__":
    sys.exit(main())
