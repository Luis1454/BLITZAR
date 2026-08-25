"""Repository profiles and file discovery for the architecture gate."""

from __future__ import annotations

import dataclasses
import json
import pathlib
import subprocess

from architecture_metrics import SOURCE_SUFFIXES


SOURCE_ROOTS = ("include", "src", "apps")
COMPILED_SUFFIXES = {".c", ".cc", ".cpp", ".cu", ".hip"}
DEFAULT_THRESHOLDS = {
    "max_parameters": 4,
    "max_function_lines": 80,
    "max_functions_per_file": 12,
    "max_branch_points": 12,
    "max_allocation_sites": 8,
    "max_internal_includes": 12,
}
DEFAULT_PROFILE_CONFIG = {
    "production": {
        "roots": ["include", "src", "apps"],
        "suffixes": sorted(SOURCE_SUFFIXES),
        "thresholds": DEFAULT_THRESHOLDS,
    },
    "tests": {
        "roots": ["tests"],
        "suffixes": sorted(SOURCE_SUFFIXES | {".cmake"}),
        "thresholds": DEFAULT_THRESHOLDS,
    },
    "examples": {
        "roots": ["examples"],
        "suffixes": sorted(SOURCE_SUFFIXES),
        "thresholds": DEFAULT_THRESHOLDS,
    },
    "tools": {
        "roots": ["tools"],
        "suffixes": [".py"],
        "thresholds": {
            **DEFAULT_THRESHOLDS,
            "max_functions_per_file": 20,
            "max_allocation_sites": 40,
        },
    },
    "build-metadata": {
        "paths": ["CMakeLists.txt", ".clang-format", ".gitignore"],
        "roots": ["cmake", ".github"],
        "suffixes": [".cmake", ".in", ".yml", ".yaml"],
        "thresholds": DEFAULT_THRESHOLDS,
    },
    "documentation": {
        "paths": ["PLAN.md", "AGENTS.md"],
        "roots": ["plan"],
        "suffixes": [".json", ".md"],
        "thresholds": DEFAULT_THRESHOLDS,
    },
}


@dataclasses.dataclass(frozen=True)
class ArchitectureProfile:
    name: str
    roots: tuple[str, ...]
    paths: tuple[str, ...]
    suffixes: tuple[str, ...]
    thresholds: dict[str, int]


def profile_thresholds(
    configured: object,
    base: dict[str, int] | None = None,
) -> dict[str, int]:
    thresholds = dict(base or DEFAULT_THRESHOLDS)
    if not isinstance(configured, dict):
        return thresholds
    for name, default in DEFAULT_THRESHOLDS.items():
        value = configured.get(name, thresholds.get(name, default))
        if isinstance(value, int) and value > 0:
            thresholds[name] = value
    return thresholds


def load_profiles(root: pathlib.Path) -> tuple[ArchitectureProfile, ...]:
    quality_path = root / "plan" / "quality.json"
    configured_profiles: object = None
    base_thresholds = dict(DEFAULT_THRESHOLDS)
    if quality_path.is_file():
        quality = json.loads(quality_path.read_text(encoding="utf-8"))
        architecture = quality.get("architecture", {})
        if isinstance(architecture, dict):
            configured_profiles = architecture.get("profiles")
            base_thresholds = profile_thresholds(architecture.get("thresholds"))

    source = configured_profiles if isinstance(configured_profiles, dict) else DEFAULT_PROFILE_CONFIG
    profiles: list[ArchitectureProfile] = []
    for name, config in source.items():
        if not isinstance(config, dict):
            raise ValueError(f"architecture profile {name} must be an object")
        roots = tuple(str(item).replace("\\", "/").strip("/") for item in config.get("roots", []))
        paths = tuple(str(item).replace("\\", "/").strip("/") for item in config.get("paths", []))
        suffixes = tuple(str(item).lower() for item in config.get("suffixes", []))
        if not roots and not paths:
            raise ValueError(f"architecture profile {name} needs roots or paths")
        profiles.append(
            ArchitectureProfile(
                name=str(name),
                roots=roots,
                paths=paths,
                suffixes=suffixes,
                thresholds=profile_thresholds(config.get("thresholds"), base_thresholds),
            )
        )
    if not profiles:
        raise ValueError("architecture profiles must not be empty")
    return tuple(profiles)


def discover_files(root: pathlib.Path) -> list[pathlib.Path]:
    return sorted(
        path
        for source_root in SOURCE_ROOTS
        for path in (root / source_root).rglob("*")
        if path.is_file() and path.suffix.lower() in SOURCE_SUFFIXES
    )


def discover_repository_paths(root: pathlib.Path) -> list[pathlib.Path]:
    try:
        result = subprocess.run(
            ["git", "-C", str(root), "ls-files", "--cached", "--others", "--exclude-standard", "-z"],
            check=False,
            capture_output=True,
        )
    except OSError:
        result = None
    if result is not None and result.returncode == 0:
        paths = [item for item in result.stdout.decode().split("\0") if item]
        return [root / pathlib.PurePosixPath(item) for item in paths]

    return sorted(
        path
        for path in root.rglob("*")
        if path.is_file() and ".git" not in path.parts
    )


def profile_matches(relative: str, profile: ArchitectureProfile) -> bool:
    normalized = relative.replace("\\", "/")
    exact_path = normalized in profile.paths
    under_root = any(
        normalized == root or normalized.startswith(f"{root}/")
        for root in profile.roots
    )
    if not exact_path and not under_root:
        return False
    if exact_path:
        return True
    return pathlib.PurePosixPath(normalized).suffix.lower() in profile.suffixes


def classify_path(relative: str, profiles: tuple[ArchitectureProfile, ...]) -> str | None:
    return next(
        (profile.name for profile in profiles if profile_matches(relative, profile)),
        None,
    )


def load_thresholds(root: pathlib.Path) -> dict[str, int]:
    quality_path = root / "plan" / "quality.json"
    if not quality_path.is_file():
        return dict(DEFAULT_THRESHOLDS)
    quality = json.loads(quality_path.read_text(encoding="utf-8"))
    architecture = quality.get("architecture", {})
    configured = architecture.get("thresholds", {}) if isinstance(architecture, dict) else {}
    return profile_thresholds(configured)
