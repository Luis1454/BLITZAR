"""Validate tracked-file and build-root workspace hygiene."""

from __future__ import annotations

import argparse
import fnmatch
import json
import pathlib
import subprocess
import sys


def load_policy(root: pathlib.Path) -> dict[str, object]:
    policy_path = root / "plan" / "workspace.json"
    policy = json.loads(policy_path.read_text(encoding="utf-8"))
    if not isinstance(policy, dict):
        raise ValueError("workspace policy must be an object")
    return policy


def tracked_paths(root: pathlib.Path) -> list[str]:
    result = subprocess.run(
        ["git", "-C", str(root), "ls-files", "-z"],
        check=False,
        capture_output=True,
    )
    if result.returncode != 0:
        raise RuntimeError(result.stderr.decode(errors="replace").strip())
    return [item for item in result.stdout.decode().split("\0") if item]


def policy_violations(root: pathlib.Path, policy: dict[str, object]) -> list[str]:
    violations: list[str] = []
    required_ignores = policy.get("required_gitignore", [])
    if not isinstance(required_ignores, list):
        return ["required_gitignore must be a list"]
    ignore_path = root / ".gitignore"
    ignore_lines = {
        line.strip() for line in ignore_path.read_text(encoding="utf-8").splitlines()
    }
    for entry in required_ignores:
        if not isinstance(entry, str) or entry not in ignore_lines:
            violations.append(f".gitignore is missing required entry: {entry}")

    external_root = policy.get("external_build_root")
    if not isinstance(external_root, str) or not external_root:
        violations.append("external_build_root must be a non-empty path")
    else:
        resolved_root = (root / pathlib.Path(external_root)).resolve()
        if resolved_root == root.resolve() or root.resolve() in resolved_root.parents:
            violations.append("external_build_root must resolve outside the repository")

    ci_root = policy.get("ci_build_root")
    if not isinstance(ci_root, str) or "${RUNNER_TEMP}" not in ci_root:
        violations.append("ci_build_root must be based on ${RUNNER_TEMP}")
    return violations


def generated_path(path: str, policy: dict[str, object]) -> bool:
    parts = pathlib.PurePosixPath(path.replace("\\", "/")).parts
    generated_components = policy.get("generated_path_components", [])
    if any(part in generated_components for part in parts):
        return True
    name = pathlib.PurePosixPath(path).name
    generated_names = policy.get("generated_file_names", [])
    if name in generated_names:
        return True
    generated_suffixes = policy.get("generated_file_suffixes", [])
    return any(fnmatch.fnmatch(name, str(pattern)) for pattern in generated_suffixes)


def inventory(root: pathlib.Path) -> dict[str, int]:
    build_directories = sum(
        1
        for path in root.iterdir()
        if path.is_dir() and (path.name == "build" or path.name.startswith("build-"))
    )
    cache_directories = sum(
        1 for path in root.rglob("__pycache__") if path.is_dir() and ".git" not in path.parts
    )
    return {
        "in_repository_build_directories": build_directories,
        "python_cache_directories": cache_directories,
    }


def validate(
    root: pathlib.Path,
    policy: dict[str, object],
    tracked: list[str],
) -> tuple[list[str], dict[str, int]]:
    violations = policy_violations(root, policy)
    violations.extend(
        f"tracked generated path is forbidden: {path}"
        for path in tracked
        if generated_path(path, policy)
    )
    return violations, inventory(root)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=pathlib.Path, required=True)
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--output", type=pathlib.Path)
    arguments = parser.parse_args(argv)
    if not arguments.check:
        parser.error("--check is required")
    root = arguments.root.resolve()
    try:
        policy = load_policy(root)
        violations, counts = validate(root, policy, tracked_paths(root))
    except (OSError, RuntimeError, ValueError, json.JSONDecodeError) as error:
        print(f"workspace-gate: {error}", file=sys.stderr)
        return 1
    report = {"status": "ok" if not violations else "invalid", "counts": counts,
              "violations": violations}
    if arguments.output is not None:
        arguments.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    for violation in violations:
        print(f"workspace-gate: {violation}", file=sys.stderr)
    print(
        "workspace-gate: "
        f"build_dirs={counts['in_repository_build_directories']}, "
        f"python_caches={counts['python_cache_directories']}, "
        f"violations={len(violations)}"
    )
    return int(bool(violations))


if __name__ == "__main__":
    sys.exit(main())
