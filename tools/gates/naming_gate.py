"""Validate repository filename, path, and primary-type naming contracts."""

from __future__ import annotations

import argparse
import json
import pathlib
import re
import sys
from collections import defaultdict

from tools.architecture.architecture_metrics import SOURCE_SUFFIXES
from tools.architecture.architecture_scope import classify_path, discover_repository_paths, load_profiles


CODE_SUFFIXES = SOURCE_SUFFIXES | {".c", ".cc", ".cpp", ".cu", ".cuh", ".hip"}
HEADER_SUFFIXES = {".h", ".hpp", ".cuh"}
IMPLEMENTATION_SUFFIXES = {".cpp", ".cc", ".c", ".cu", ".hip"}
PASCAL_CPP_PATTERN = re.compile(r"[A-Z][A-Za-z0-9]*\.(?:cpp|cc|cu|cuh|hip|hpp|inl)$")
PASCAL_C_PATTERN = re.compile(r"[A-Z][A-Za-z0-9]*\.(?:c|h)$")
INCLUDE_PATTERN = re.compile(r"^\s*#\s*include\s*[\"]([^\"]+)[\"]", re.MULTILINE)
TYPE_PATTERN = re.compile(r"\b(?:class|struct|enum class|enum)\s+([A-Za-z_]\w*)")

DEFAULT_POLICY = {
    "max_filename_length": {
        "production": 32,
        "tests": 32,
        "examples": 32,
        "tools": 32,
        "build-metadata": 48,
        "documentation": 64,
    },
    "allowed_stem_pairs": [
        {"extensions": [".cpp", ".hpp"], "reason": "implementation/header pair"},
        {"extensions": [".cc", ".h"], "reason": "implementation/header pair"},
        {"extensions": [".cu", ".cuh"], "reason": "CUDA implementation/header pair"},
        {"extensions": [".hip", ".hpp"], "reason": "HIP implementation/header pair"},
        {"extensions": [".h", ".hpp"], "reason": "public ABI header pair"},
    ],
    "name_exceptions": [],
    "type_mapping_exceptions": [],
    "standalone_implementation_exceptions": [],
    "forbidden_path_components": ["utils", "common", "misc", "private", "details"],
    "forbidden_name_prefixes": [
        "Srv",
        "Server",
        "Legacy",
        "Old",
        "Temp",
        "Utils",
        "Common",
        "Misc",
        "Private",
        "Details",
    ],
}


def load_policy(root: pathlib.Path) -> tuple[dict[str, object], bool]:
    quality_path = root / "plan" / "quality.json"
    if not quality_path.is_file():
        return DEFAULT_POLICY, False
    quality = json.loads(quality_path.read_text(encoding="utf-8"))
    configured = quality.get("naming")
    if not isinstance(configured, dict):
        architecture = quality.get("architecture", {})
        configured = architecture.get("naming") if isinstance(architecture, dict) else None
    if not isinstance(configured, dict):
        return DEFAULT_POLICY, False
    policy = dict(DEFAULT_POLICY)
    policy.update(configured)
    return policy, True


def code_files(root: pathlib.Path) -> list[pathlib.Path]:
    return sorted(
        path
        for path in discover_repository_paths(root)
        if path.suffix.lower() in CODE_SUFFIXES
        and any(path.relative_to(root).as_posix().startswith(f"{item}/") for item in (
            "include",
            "src",
            "apps",
            "tests",
            "examples",
        ))
    )


def path_key(path: pathlib.Path, root: pathlib.Path) -> str:
    return path.relative_to(root).as_posix()


def configured_exception_map(policy: dict[str, object], key: str) -> dict[str, str]:
    values = policy.get(key, [])
    if not isinstance(values, list):
        return {}
    result: dict[str, str] = {}
    for item in values:
        if isinstance(item, dict) and isinstance(item.get("path"), str):
            result[item["path"]] = str(item.get("reason", "documented exception"))
    return result


def max_lengths(policy: dict[str, object]) -> dict[str, int]:
    configured = policy.get("max_filename_length", {})
    values = dict(DEFAULT_POLICY["max_filename_length"])
    if isinstance(configured, dict):
        for category, value in configured.items():
            if isinstance(value, int) and value > 0:
                values[str(category)] = value
    return values


def duplicate_names(files: list[pathlib.Path], root: pathlib.Path) -> list[dict[str, object]]:
    groups: dict[str, list[str]] = defaultdict(list)
    for path in files:
        groups[path.name.casefold()].append(path_key(path, root))
    return [
        {"name": name, "paths": sorted(paths)}
        for name, paths in sorted(groups.items())
        if len(paths) > 1
    ]


def duplicate_stems(
    files: list[pathlib.Path],
    root: pathlib.Path,
    policy: dict[str, object],
) -> list[dict[str, object]]:
    groups: dict[str, list[pathlib.Path]] = defaultdict(list)
    for path in files:
        groups[path.stem.casefold()].append(path)
    pairs = {
        tuple(sorted(str(extension).lower() for extension in item.get("extensions", [])))
        for item in policy.get("allowed_stem_pairs", [])
        if isinstance(item, dict) and isinstance(item.get("extensions"), list)
    }
    collisions: list[dict[str, object]] = []
    for stem, paths in sorted(groups.items()):
        if len(paths) < 2:
            continue
        extensions = tuple(sorted(path.suffix.lower() for path in paths))
        allowed = len(paths) == 2 and extensions in pairs and paths[0].parent == paths[1].parent
        collisions.append(
            {
                "stem": stem,
                "paths": sorted(path_key(path, root) for path in paths),
                "status": "allowed_pair" if allowed else "violation",
            }
        )
    return collisions


def filename_report(
    root: pathlib.Path,
    files: list[pathlib.Path],
    policy: dict[str, object],
    profiles: tuple[object, ...],
) -> dict[str, object]:
    name_exceptions = configured_exception_map(policy, "name_exceptions")
    lengths = max_lengths(policy)
    pascal_violations: list[str] = []
    long_names: list[dict[str, object]] = []
    forbidden_paths: list[str] = []
    forbidden_prefixes: list[str] = []
    applied_exceptions: list[dict[str, str]] = []
    forbidden_components = {
        str(item).casefold() for item in policy.get("forbidden_path_components", [])
    }
    forbidden_names = [str(item) for item in policy.get("forbidden_name_prefixes", [])]

    for path in files:
        relative = path_key(path, root)
        category = classify_path(relative, profiles) or "production"
        exception = name_exceptions.get(relative)
        if exception is not None:
            applied_exceptions.append({"path": relative, "rule": "name", "reason": exception})
        elif path.suffix.lower() in {".cpp", ".cc", ".cu", ".cuh", ".hip", ".hpp", ".inl"}:
            if PASCAL_CPP_PATTERN.fullmatch(path.name) is None:
                pascal_violations.append(relative)
        elif path.suffix.lower() in {".c", ".h"} and PASCAL_C_PATTERN.fullmatch(path.name) is None:
            pascal_violations.append(relative)

        limit = lengths.get(category)
        if limit is not None and len(path.name) > limit:
            long_names.append({"path": relative, "length": len(path.name), "limit": limit})
        if any(part.casefold() in forbidden_components for part in path.parts[:-1]):
            forbidden_paths.append(relative)
        for prefix in forbidden_names:
            if path.stem.casefold().startswith(prefix.casefold()):
                forbidden_prefixes.append(relative)
                break

    return {
        "full_filename_duplicates": duplicate_names(files, root),
        "stem_collisions": duplicate_stems(files, root, policy),
        "pascal_case_violations": sorted(pascal_violations),
        "long_names": sorted(long_names, key=lambda item: str(item["path"])),
        "forbidden_paths": sorted(forbidden_paths),
        "forbidden_prefixes": sorted(forbidden_prefixes),
        "declared_exceptions": [
            {"path": path, "rule": "name", "reason": reason}
            for path, reason in sorted(name_exceptions.items())
        ],
        "applied_exceptions": applied_exceptions,
    }


def resolve_include(root: pathlib.Path, source: pathlib.Path, include: str) -> pathlib.Path | None:
    candidates = [source.parent / include, root / "src" / include, root / include]
    for candidate in candidates:
        if candidate.is_file():
            return candidate
    return None


def type_report(
    root: pathlib.Path,
    files: list[pathlib.Path],
    policy: dict[str, object],
    enforce: bool,
) -> dict[str, object]:
    type_exceptions = configured_exception_map(policy, "type_mapping_exceptions")
    standalone_exceptions = configured_exception_map(
        policy, "standalone_implementation_exceptions"
    )
    matched: list[str] = []
    contract_headers: list[str] = []
    exceptions: list[dict[str, str]] = []
    violations: list[str] = []
    implementation_violations: list[str] = []

    for path in files:
        relative = path_key(path, root)
        source = path.read_text(encoding="utf-8", errors="ignore")
        suffix = path.suffix.lower()
        if suffix in HEADER_SUFFIXES:
            types = {item.casefold() for item in TYPE_PATTERN.findall(source)}
            if path.stem.casefold() in types:
                matched.append(relative)
            elif relative in type_exceptions:
                exceptions.append({"path": relative, "reason": type_exceptions[relative]})
            elif not types or len(types) > 1:
                contract_headers.append(relative)
            elif enforce:
                violations.append(relative)
        elif enforce and suffix in IMPLEMENTATION_SUFFIXES and relative.startswith("src/"):
            includes = [resolve_include(root, path, item) for item in INCLUDE_PATTERN.findall(source)]
            if not any(item is not None for item in includes) and relative not in standalone_exceptions:
                implementation_violations.append(relative)

    return {
        "matched": sorted(matched),
        "contract_headers": sorted(contract_headers),
        "exceptions": exceptions,
        "violations": sorted(violations),
        "implementation_violations": sorted(implementation_violations),
    }


def build_report(root: pathlib.Path) -> dict[str, object]:
    policy, enforce_types = load_policy(root)
    profiles = load_profiles(root)
    files = code_files(root)
    names = filename_report(root, files, policy, profiles)
    types = type_report(root, files, policy, enforce_types)
    return {
        "schema_version": 1,
        "file_count": len(files),
        "max_filename_length": max_lengths(policy),
        "name_report": names,
        "type_report": types,
        "enforce_type_mapping": enforce_types,
    }


def violations(report: dict[str, object]) -> list[str]:
    names = report["name_report"]
    failures: list[str] = []
    for item in names["full_filename_duplicates"]:
        failures.append(f"duplicate source filename: {', '.join(item['paths'])}")
    for item in names["stem_collisions"]:
        if item["status"] == "violation":
            failures.append(f"duplicate source stem: {', '.join(item['paths'])}")
    failures.extend(f"non-PascalCase source filename: {path}" for path in names["pascal_case_violations"])
    failures.extend(f"source filename exceeds category limit: {item['path']}" for item in names["long_names"])
    failures.extend(f"forbidden repository path component: {path}" for path in names["forbidden_paths"])
    failures.extend(f"forbidden redundant filename prefix: {path}" for path in names["forbidden_prefixes"])
    types = report["type_report"]
    failures.extend(f"primary type/file mismatch: {path}" for path in types["violations"])
    failures.extend(f"implementation has no owner header: {path}" for path in types["implementation_violations"])
    return failures


def validate(root: pathlib.Path) -> list[str]:
    return violations(build_report(root))


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=pathlib.Path, default=pathlib.Path(__file__).resolve().parents[2])
    parser.add_argument("--output", type=pathlib.Path)
    parser.add_argument("--check", action="store_true")
    arguments = parser.parse_args(argv)
    root = arguments.root.resolve()
    try:
        report = build_report(root)
        failures = violations(report)
    except (OSError, ValueError, json.JSONDecodeError, KeyError, TypeError) as error:
        print(f"naming-gate: {error}", file=sys.stderr)
        return 1
    if arguments.output is not None:
        arguments.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    elif not arguments.check:
        print(json.dumps(report, indent=2))
    if arguments.check and failures:
        for failure in failures:
            print(f"naming-gate: {failure}", file=sys.stderr)
        return 1
    print(f"naming-gate: {report['file_count']} source files, {len(failures)} violations", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
