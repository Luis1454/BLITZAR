"""Validate the frozen repository taxonomy and its eventual materialization."""

from __future__ import annotations

import argparse
import json
import pathlib
import re
import sys
from typing import Any


SCHEMA_VERSION = 1
MATERIALIZATION_STATES = {"planned", "materialized"}
TEST_EXTENSIONS = {".c", ".cpp", ".cmake"}
LANGUAGE_EXTENSIONS = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx"}
PYTHON_MODULE_PATTERN = re.compile(r"[a-z][a-z0-9_]*\.py$")
PYTHON_DOMAIN_PATTERN = re.compile(r"[a-z][a-z0-9_]*$")
IDENTIFIER_PATTERN = re.compile(r"[a-z][a-z0-9_]*$")
PATH_PART_PATTERN = re.compile(r"(?:[A-Za-z0-9][A-Za-z0-9_.-]*|\.[A-Za-z0-9][A-Za-z0-9_.-]*)$")
VERSION_PATTERN = re.compile(r"[0-9]+\.[0-9]+\.[0-9]+$")


def load_policy(root: pathlib.Path) -> dict[str, Any]:
    path = root / "plan" / "repository_tree.json"
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise ValueError(f"repository tree policy is not valid JSON: {error}") from error
    if not isinstance(value, dict):
        raise ValueError("repository tree policy must be a JSON object")
    return value


def _path(value: object, label: str, root_name: str | None = None) -> pathlib.PurePosixPath:
    if not isinstance(value, str) or not value:
        raise ValueError(f"{label} must be a non-empty relative path")
    if "\\" in value:
        raise ValueError(f"{label} must use forward slashes: {value}")
    path = pathlib.PurePosixPath(value)
    if path.is_absolute() or any(part in {"", ".", ".."} for part in value.split("/")):
        raise ValueError(f"{label} contains an unsafe path: {value}")
    if any(PATH_PART_PATTERN.fullmatch(part) is None for part in path.parts):
        raise ValueError(f"{label} contains an invalid path component: {value}")
    if root_name is not None and (not path.parts or path.parts[0] != root_name):
        raise ValueError(f"{label} must be below {root_name}/: {value}")
    return path


def _is_below(path: pathlib.PurePosixPath, parent: pathlib.PurePosixPath) -> bool:
    return path == parent or parent in path.parents


def _strictly_below(path: pathlib.PurePosixPath, parent: pathlib.PurePosixPath) -> bool:
    return parent in path.parents


def _overlap(left: pathlib.PurePosixPath, right: pathlib.PurePosixPath) -> bool:
    return left == right or left in right.parents or right in left.parents


def _directories(root: pathlib.Path) -> set[pathlib.PurePosixPath]:
    result = {pathlib.PurePosixPath(".")}
    result.update(
        pathlib.PurePosixPath(path.relative_to(root).as_posix())
        for path in root.rglob("*")
        if path.is_dir()
    )
    return result


def _validate_aliases(policy: dict[str, Any], errors: list[str]) -> None:
    aliases = policy.get("aliases")
    if not isinstance(aliases, dict) or not aliases:
        errors.append("aliases must be a non-empty object")
        return
    values: set[str] = set()
    for key, value in aliases.items():
        if (
            not isinstance(key, str)
            or IDENTIFIER_PATTERN.fullmatch(key) is None
            or not isinstance(value, str)
            or IDENTIFIER_PATTERN.fullmatch(value) is None
        ):
            errors.append(f"alias must use lowercase identifiers: {key} -> {value}")
            continue
        if value in values:
            errors.append(f"alias values must be unique: {value}")
        values.add(value)


def _validate_language_separation(policy: dict[str, Any], errors: list[str]) -> list[dict[str, Any]]:
    configured = policy.get("language_separation")
    if not isinstance(configured, list) or not configured:
        errors.append("language_separation must be a non-empty list")
        return []

    result: list[dict[str, Any]] = []
    roots: list[pathlib.PurePosixPath] = []
    for index, item in enumerate(configured):
        if not isinstance(item, dict):
            errors.append(f"language_separation entry {index} must be an object")
            continue
        try:
            root = _path(item.get("root"), f"language_separation {index} root")
        except ValueError as error:
            errors.append(str(error))
            continue
        if root.parts[0] not in {"include", "tests", "examples"}:
            errors.append(f"language separation root is outside public/test/example trees: {root}")
        if any(_overlap(root, previous) for previous in roots):
            errors.append(f"language separation roots must not overlap: {root}")
        roots.append(root)

        children = item.get("children")
        if not isinstance(children, list) or not children:
            errors.append(f"language separation children are missing: {root}")
            continue
        normalized_children: list[str] = []
        for child in children:
            if not isinstance(child, str) or IDENTIFIER_PATTERN.fullmatch(child) is None:
                errors.append(f"language separation child must be a lowercase identifier: {root}/{child}")
                continue
            normalized_children.append(child)
        if len(normalized_children) != len(set(normalized_children)):
            errors.append(f"language separation children must be unique: {root}")
        result.append({"root": root, "children": tuple(normalized_children)})
    return result


def _validate_source_test_pairs(policy: dict[str, Any], errors: list[str]) -> list[tuple[pathlib.PurePosixPath, pathlib.PurePosixPath]]:
    configured = policy.get("source_test_pairs")
    if not isinstance(configured, list) or not configured:
        errors.append("source_test_pairs must be a non-empty list")
        return []

    pairs: list[tuple[pathlib.PurePosixPath, pathlib.PurePosixPath]] = []
    for index, item in enumerate(configured):
        if not isinstance(item, dict):
            errors.append(f"source_test_pairs entry {index} must be an object")
            continue
        try:
            source = _path(item.get("source"), f"source_test_pairs {index} source", "src")
            tests = _path(item.get("tests"), f"source_test_pairs {index} tests", "tests")
        except ValueError as error:
            errors.append(str(error))
            continue
        responsibility = item.get("responsibility")
        if not isinstance(responsibility, str) or not responsibility.strip():
            errors.append(f"source_test_pairs {index} needs a responsibility")
        if any(source == old_source and tests == old_tests for old_source, old_tests in pairs):
            errors.append(f"duplicate source/test pair: {source} -> {tests}")
        if any(_overlap(source, old_source) for old_source, _ in pairs):
            errors.append(f"source/test source roots must not overlap: {source}")
        if any(_overlap(tests, old_tests) for _, old_tests in pairs):
            errors.append(f"source/test test roots must not overlap: {tests}")
        pairs.append((source, tests))
    return pairs


def _validate_allowed_missing(
    policy: dict[str, Any],
    pairs: list[tuple[pathlib.PurePosixPath, pathlib.PurePosixPath]],
    errors: list[str],
) -> set[tuple[pathlib.PurePosixPath, pathlib.PurePosixPath]]:
    configured = policy.get("allowed_missing")
    if not isinstance(configured, list):
        errors.append("allowed_missing must be a list")
        return set()

    allowed: set[tuple[pathlib.PurePosixPath, pathlib.PurePosixPath]] = set()
    for index, item in enumerate(configured):
        if not isinstance(item, dict):
            errors.append(f"allowed_missing entry {index} must be an object")
            continue
        try:
            source = _path(item.get("source"), f"allowed_missing {index} source", "src")
            tests = _path(item.get("tests"), f"allowed_missing {index} tests", "tests")
        except ValueError as error:
            errors.append(str(error))
            continue
        reason = item.get("reason")
        if not isinstance(reason, str) or not reason.strip():
            errors.append(f"allowed_missing entry {index} needs a non-empty reason")
        matches = [
            (source_root, tests_root)
            for source_root, tests_root in pairs
            if _strictly_below(source, source_root)
            and _strictly_below(tests, tests_root)
            and source.relative_to(source_root) == tests.relative_to(tests_root)
        ]
        if len(matches) != 1:
            errors.append(f"allowed_missing entry is outside exactly one source/test pair: {source} -> {tests}")
        key = (source, tests)
        if key in allowed:
            errors.append(f"duplicate allowed_missing entry: {source} -> {tests}")
        allowed.add(key)
    return allowed


def _validate_test_entries(policy: dict[str, Any], errors: list[str]) -> tuple[set[pathlib.PurePosixPath], set[pathlib.PurePosixPath]]:
    suffix = policy.get("test_suffix")
    if not isinstance(suffix, dict):
        errors.append("test_suffix must be an object")
        return set(), set()
    if suffix.get("suffix") != "Test":
        errors.append("test_suffix.suffix must remain Test")
    extensions = suffix.get("extensions")
    if (
        not isinstance(extensions, list)
        or sorted(extensions) != sorted(TEST_EXTENSIONS)
        or len(set(extensions)) != len(extensions)
    ):
        errors.append("test_suffix.extensions must be .c, .cmake, and .cpp")

    targets = policy.get("target_test_entrypoints")
    if not isinstance(targets, list) or not targets:
        errors.append("target_test_entrypoints must be a non-empty list")
        targets = []
    target_paths: set[pathlib.PurePosixPath] = set()
    for value in targets:
        try:
            path = _path(value, "target_test_entrypoint", "tests")
        except ValueError as error:
            errors.append(str(error))
            continue
        if path.suffix.lower() not in TEST_EXTENSIONS:
            errors.append(f"target test entrypoint has unsupported extension: {path}")
        stem = path.name[: -len(path.suffix)] if path.suffix else path.name
        if not stem.endswith("Test"):
            errors.append(f"target test entrypoint must end in Test: {path}")
        if path in target_paths:
            errors.append(f"duplicate target test entrypoint: {path}")
        target_paths.add(path)

    non_tests = policy.get("test_suffix", {}).get("non_test_entrypoints", [])
    if not isinstance(non_tests, list):
        errors.append("test_suffix.non_test_entrypoints must be a list")
        non_tests = []
    non_test_paths: set[pathlib.PurePosixPath] = set()
    for value in non_tests:
        try:
            path = _path(value, "non_test_entrypoint")
        except ValueError as error:
            errors.append(str(error))
            continue
        if path.parts[0] == "tests":
            errors.append(f"non-test entrypoint cannot be below tests/: {path}")
        if path in non_test_paths:
            errors.append(f"duplicate non-test entrypoint: {path}")
        if path in target_paths:
            errors.append(f"entrypoint cannot be both test and non-test: {path}")
        non_test_paths.add(path)
    return target_paths, non_test_paths


def _validate_python(policy: dict[str, Any], errors: list[str]) -> dict[str, Any] | None:
    configured = policy.get("python")
    if not isinstance(configured, dict):
        errors.append("python policy must be an object")
        return None
    try:
        root = _path(configured.get("root"), "python.root", "tools")
    except ValueError as error:
        errors.append(str(error))
        root = pathlib.PurePosixPath("tools")
    domains = configured.get("domains")
    if not isinstance(domains, list) or not domains:
        errors.append("python.domains must be a non-empty list")
        domains = []
    normalized_domains: list[str] = []
    for domain in domains:
        if not isinstance(domain, str) or PYTHON_DOMAIN_PATTERN.fullmatch(domain) is None:
            errors.append(f"python domain must be lowercase: {domain}")
            continue
        normalized_domains.append(domain)
    if len(normalized_domains) != len(set(normalized_domains)):
        errors.append("python domains must be unique")
    test_suffix = configured.get("test_suffix")
    if not isinstance(test_suffix, str) or not test_suffix.endswith(".py") or not test_suffix.startswith("_"):
        errors.append("python.test_suffix must be a lowercase underscore-prefixed .py suffix")
    policy_text = configured.get("domain_policy")
    if not isinstance(policy_text, str) or not policy_text.strip():
        errors.append("python.domain_policy must be a non-empty description")
    return {"root": root, "domains": tuple(normalized_domains), "test_suffix": test_suffix}


def _validate_forbidden_and_migration(policy: dict[str, Any], errors: list[str]) -> None:
    forbidden = policy.get("forbidden_paths")
    if not isinstance(forbidden, list) or not forbidden:
        errors.append("forbidden_paths must be a non-empty list")
    else:
        seen: set[pathlib.PurePosixPath] = set()
        for value in forbidden:
            try:
                path = _path(value, "forbidden_path")
            except ValueError as error:
                errors.append(str(error))
                continue
            if path in seen:
                errors.append(f"duplicate forbidden path: {path}")
            seen.add(path)

    migration = policy.get("migration")
    if not isinstance(migration, dict):
        errors.append("migration must be an object")
        return
    materialization = policy.get("materialization")
    if migration.get("state") != materialization:
        errors.append("migration.state must match materialization")
    if not isinstance(migration.get("rule"), str) or not migration["rule"].strip():
        errors.append("migration.rule must be a non-empty description")
    moves = migration.get("directory_moves")
    if not isinstance(moves, list) or not moves:
        errors.append("migration.directory_moves must be a non-empty list")
        return
    sources: set[pathlib.PurePosixPath] = set()
    targets: set[pathlib.PurePosixPath] = set()
    for index, move in enumerate(moves):
        if not isinstance(move, list) or len(move) != 2:
            errors.append(f"migration directory move {index} must contain source and target")
            continue
        try:
            source = _path(move[0], f"migration move {index} source")
            target = _path(move[1], f"migration move {index} target")
        except ValueError as error:
            errors.append(str(error))
            continue
        if source == target:
            errors.append(f"migration move {index} must change the path: {source}")
        if source in sources:
            errors.append(f"duplicate migration source: {source}")
        if target in targets:
            errors.append(f"duplicate migration target: {target}")
        sources.add(source)
        targets.add(target)


def validate_policy(root: pathlib.Path) -> list[str]:
    """Validate the policy without requiring its planned paths to exist."""

    policy = load_policy(root)
    errors: list[str] = []
    if policy.get("schema_version") != SCHEMA_VERSION:
        errors.append(f"schema_version must be {SCHEMA_VERSION}")
    plan_version = policy.get("plan_version")
    if not isinstance(plan_version, str) or VERSION_PATTERN.fullmatch(plan_version) is None:
        errors.append("plan_version must be a semantic version")
    if policy.get("status") != "frozen":
        errors.append("status must remain frozen")
    if policy.get("materialization") not in MATERIALIZATION_STATES:
        errors.append("materialization must be planned or materialized")

    _validate_aliases(policy, errors)
    _validate_language_separation(policy, errors)
    pairs = _validate_source_test_pairs(policy, errors)
    _validate_allowed_missing(policy, pairs, errors)
    target_paths, non_test_paths = _validate_test_entries(policy, errors)
    python_policy = _validate_python(policy, errors)
    _validate_forbidden_and_migration(policy, errors)

    if target_paths & non_test_paths:
        errors.append("test and non-test entrypoint sets must be disjoint")
    if not target_paths:
        errors.append("at least one target test entrypoint is required")
    if python_policy is None:
        errors.append("python policy cannot be evaluated")
    return errors


def validate_plan(root: pathlib.Path) -> list[str]:
    """Validate the frozen policy and its link to the repository manifest."""

    errors = validate_policy(root)
    manifest_path = root / "plan" / "manifest.json"
    if not manifest_path.is_file():
        errors.append("plan/manifest.json is missing")
        return errors
    try:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        errors.append(f"manifest is not valid JSON: {error}")
        return errors
    if not isinstance(manifest, dict):
        errors.append("manifest must be a JSON object")
        return errors
    policy = load_policy(root)
    if manifest.get("plan_version") != policy.get("plan_version"):
        errors.append("repository tree plan_version does not match manifest")
    source_of_truth = manifest.get("source_of_truth")
    if not isinstance(source_of_truth, list) or "plan/repository_tree.json" not in source_of_truth:
        errors.append("manifest source_of_truth must include plan/repository_tree.json")
    return errors


def _validate_materialized_symmetry(
    root: pathlib.Path,
    pairs: list[tuple[pathlib.PurePosixPath, pathlib.PurePosixPath]],
    allowed: set[tuple[pathlib.PurePosixPath, pathlib.PurePosixPath]],
) -> list[str]:
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
        source_directories = _directories(source_root)
        test_directories = _directories(tests_root)
        for relative in sorted(source_directories - test_directories):
            source_path = source_relative / relative
            tests_path = tests_relative / relative
            if (source_path, tests_path) in allowed:
                consumed.add((source_path, tests_path))
            else:
                violations.append(f"missing mirrored test directory: {source_path} -> {tests_path}")
        for relative in sorted(test_directories - source_directories):
            source_path = source_relative / relative
            tests_path = tests_relative / relative
            violations.append(f"extra mirrored test directory: {tests_path} -> {source_path}")
    for source_path, tests_path in sorted(allowed - consumed):
        violations.append(f"stale allowed missing directory: {source_path} -> {tests_path}")
    return violations


def validate_materialized(root: pathlib.Path) -> list[str]:
    """Validate the physical tree after the migration is deliberately promoted."""

    policy = load_policy(root)
    errors = validate_plan(root)
    if policy.get("materialization") != "materialized":
        errors.append("materialization is planned; promote the policy only with the complete tree")
        return errors

    language_entries = _validate_language_separation(policy, errors)
    pairs = _validate_source_test_pairs(policy, errors)
    allowed = _validate_allowed_missing(policy, pairs, errors)
    target_paths, non_test_paths = _validate_test_entries(policy, errors)
    python_policy = _validate_python(policy, errors)
    if errors:
        return errors

    for entry in language_entries:
        language_root = root.joinpath(*entry["root"].parts)
        if not language_root.is_dir():
            errors.append(f"language separation root is missing: {entry['root']}")
            continue
        children = set(entry["children"])
        for child in children:
            if not (language_root / child).is_dir():
                errors.append(f"language separation child is missing: {entry['root']}/{child}")
        for path in language_root.rglob("*"):
            if not path.is_file() or path.suffix.lower() not in LANGUAGE_EXTENSIONS:
                continue
            relative = path.relative_to(language_root)
            if not relative.parts or relative.parts[0] not in children:
                errors.append(f"language-mixed file is outside its language child: {path.relative_to(root)}")

    errors.extend(_validate_materialized_symmetry(root, pairs, allowed))

    tests_root = root / "tests"
    if tests_root.is_dir():
        for target in sorted(target_paths):
            if not (root / pathlib.Path(*target.parts)).is_file():
                errors.append(f"target test entrypoint is missing: {target}")

    if python_policy is not None:
        python_root = root.joinpath(*python_policy["root"].parts)
        if not python_root.is_dir():
            errors.append(f"python root is missing: {python_policy['root']}")
        else:
            domains = set(python_policy["domains"])
            for domain in domains:
                if not (python_root / domain).is_dir():
                    errors.append(f"python domain is missing: {python_policy['root']}/{domain}")
            for path in python_root.rglob("*.py"):
                relative = path.relative_to(python_root)
                if path.name == "__init__.py":
                    continue
                if not relative.parts or relative.parts[0] not in domains:
                    errors.append(f"flat cross-domain Python module: {path.relative_to(root)}")
                if PYTHON_MODULE_PATTERN.fullmatch(path.name) is None:
                    errors.append(f"Python module must use lowercase naming: {path.relative_to(root)}")
                if path.stem.endswith("test") and not path.name.endswith(str(python_policy["test_suffix"])):
                    errors.append(f"Python test must end in {python_policy['test_suffix']}: {path.relative_to(root)}")

    forbidden = policy.get("forbidden_paths", [])
    forbidden_paths = set()
    for value in forbidden if isinstance(forbidden, list) else []:
        try:
            forbidden_paths.add(_path(value, "forbidden_path"))
        except ValueError:
            continue
    for forbidden_path in sorted(forbidden_paths):
        if len(forbidden_path.parts) == 1:
            matches = sorted(root.rglob(forbidden_path.name))
            errors.extend(
                f"forbidden repository path is materialized: {path.relative_to(root)}"
                for path in matches
            )
        elif (root / pathlib.Path(*forbidden_path.parts)).exists():
            errors.append(f"forbidden repository path is materialized: {forbidden_path}")
    return errors


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=pathlib.Path, required=True)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--plan", action="store_true", help="validate the frozen target policy")
    mode.add_argument("--check", action="store_true", help="validate the promoted physical tree")
    arguments = parser.parse_args(argv)
    root = arguments.root.resolve()
    try:
        errors = validate_plan(root) if arguments.plan else validate_materialized(root)
    except (OSError, ValueError, json.JSONDecodeError, KeyError, TypeError) as error:
        print(f"repository-tree-gate: {error}", file=sys.stderr)
        return 1
    for error in errors:
        print(f"repository-tree-gate: {error}", file=sys.stderr)
    if errors:
        return 1
    state = "plan" if arguments.plan else "materialized tree"
    print(f"repository-tree-gate: {state} is valid")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
